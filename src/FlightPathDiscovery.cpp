#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Creature.h"
#include "GameObject.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "DBCStores.h"
#include "MapMgr.h"
#include "Map.h"
#include "Config.h"
#include <cmath>
#include <sstream>
#include <map>
#include <mutex>
#include "ObjectAccessor.h"
#include <set>
#include <vector>
#include <limits>
#include <algorithm>
#include <cctype>
#include <string>

namespace FPD
{
constexpr uint32 SENDER = 940100;
constexpr uint32 CONFIRM_BASE = 1000;
constexpr uint32 PURCHASE_BASE = 4000;
constexpr uint32 BACK = 1;
constexpr uint32 MAX_NODE = 1024;
constexpr uint32 GOSSIP_TEXT = 1;

bool Enabled() { return sConfigMgr->GetOption<bool>("FlightPathDiscovery.Enable", true); }
uint32 BoardEntry() { return sConfigMgr->GetOption<uint32>("FlightPathDiscovery.BoardEntry", 900100); }
bool SupportedMap(uint32 map)
{
    return map == 0 || map == 1 || (map == 571 && sConfigMgr->GetOption<bool>("FlightPathDiscovery.EnableNorthrend", false));
}

bool FactionNode(Player* player, uint32 node)
{
    if (!node || node > 256) return false; // 3.3.5a taxi mask is 8 x uint32
    TaxiMask const& mask = player->GetTeamId() == TEAM_ALLIANCE ? sAllianceTaxiNodesMask : sHordeTaxiNodesMask;
    return (mask[(node - 1) / 32] & (uint32(1) << ((node - 1) % 32))) != 0;
}

uint32 NodeFor(Creature* npc, Player* player)
{
    if (!npc || !player || !SupportedMap(npc->GetMapId()) || !npc->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_FLIGHTMASTER)) return 0;
    uint32 node = sObjectMgr->GetNearestTaxiNode(*npc, player->GetTeamId(true));
    auto const* info = sTaxiNodesStore.LookupEntry(node);
    if (!info || info->map_id != npc->GetMapId() || !FactionNode(player, node)) return 0;
    float dx = npc->GetPositionX() - info->x, dy = npc->GetPositionY() - info->y;
    return dx * dx + dy * dy < 10000.0f ? node : 0;
}

std::mutex associationsLock;
std::map<uint64, ObjectGuid> associations;
std::map<uint64, ObjectGuid> masterBoards;

Creature* MasterFor(Player* player, GameObject* board)
{
    if (!player || !board || board->GetEntry() != BoardEntry() || !SupportedMap(board->GetMapId()) || !player->IsWithinDistInMap(board, 6.0f)) return nullptr;
    ObjectGuid masterGuid;
    {
        std::lock_guard<std::mutex> guard(associationsLock);
        auto it = associations.find(board->GetGUID().GetRawValue());
        if (it == associations.end()) return nullptr;
        masterGuid = it->second;
    }
    Creature* npc = ObjectAccessor::GetCreature(*board, masterGuid);
    if (!npc || !npc->IsWithinDistInMap(board, 7.0f) || !player->IsWithinDistInMap(npc, 9.0f)) return nullptr;
    return npc;
}

// Configuration is the authoritative source for routes and destination prices.
// Keys are FlightPathDiscovery.<Continent>.Routes.<source> and .Price.<node>.
// Read once on startup. Restart worldserver after changing the configuration.
std::map<uint32, std::set<uint32>> routes;
std::map<uint32, uint32> prices;

std::string Trim(std::string value)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool ParseNode(std::string const& input, uint32& node)
{
    std::string value = Trim(input);
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }))
        return false;
    try
    {
        unsigned long long parsed = std::stoull(value);
        if (!parsed || parsed > 256 || !sTaxiNodesStore.LookupEntry(uint32(parsed))) return false;
        node = uint32(parsed);
        return true;
    }
    catch (...) { return false; }
}

void LoadNetwork()
{
    routes.clear();
    prices.clear();
    for (auto const& continent : { std::pair<char const*, uint32>{"Kalimdor", 1}, {"EasternKingdoms", 0} })
    {
        std::string prefix = std::string("FlightPathDiscovery.") + continent.first + ".";
        for (uint32 source = 1; source <= 256; ++source)
        {
            std::string key = prefix + "Routes." + std::to_string(source);
            std::string configured = sConfigMgr->GetOption<std::string>(key, "");
            if (configured.empty()) continue;
            auto const* sourceInfo = sTaxiNodesStore.LookupEntry(source);
            if (!sourceInfo || sourceInfo->map_id != continent.second) continue;
            std::stringstream input(configured);
            std::string token;
            while (std::getline(input, token, ','))
            {
                uint32 destination = 0;
                if (!ParseNode(token, destination) || destination == source) continue;
                auto const* destInfo = sTaxiNodesStore.LookupEntry(destination);
                if (!destInfo || destInfo->map_id != continent.second) continue;
                routes[source].insert(destination);
            }
        }
        for (uint32 node = 1; node <= 256; ++node)
        {
            auto const* info = sTaxiNodesStore.LookupEntry(node);
            if (!info || info->map_id != continent.second) continue;
            std::string key = prefix + "Price." + std::to_string(node);
            // -1 means no override. Zero and values > INT32_MAX are rejected.
            int64 value = sConfigMgr->GetOption<int64>(key, -1);
            if (value > 0 && value <= std::numeric_limits<int32>::max())
                prices[node] = uint32(value);
        }
    }
}

uint32 Price(uint32 destination)
{
    auto it = prices.find(destination);
    return it == prices.end() ? 0 : it->second;
}

bool ApprovedNeighbor(uint32 source, uint32 destination)
{
    auto it = routes.find(source);
    return it != routes.end() && it->second.count(destination) != 0;
}

std::string Money(uint32 copper)
{
    std::ostringstream out;
    if (copper >= 10000) out << copper / 10000 << "g ";
    if ((copper / 100) % 100) out << (copper / 100) % 100 << "s ";
    if (copper % 100) out << copper % 100 << "c";
    return out.str();
}

bool Eligible(Player* player, uint32 source, uint32 destination)
{
    if (!player || !source || !destination || !FactionNode(player, destination) || player->m_taxi.IsTaximaskNodeKnown(destination)) return false;
    auto const* dst = sTaxiNodesStore.LookupEntry(destination);
    if (!dst || !SupportedMap(dst->map_id) || !Price(destination)) return false;
    auto const* src = sTaxiNodesStore.LookupEntry(source);
    if (!src || src->map_id != dst->map_id) return false;
    return ApprovedNeighbor(source, destination);
}

void Menu(Player* player, GameObject* board, uint32 source)
{
    ClearGossipMenuFor(player);
    auto it = routes.find(source);
    if (it != routes.end())
    {
        for (uint32 dest : it->second)
        {
            if (dest >= MAX_NODE || !Eligible(player, source, dest)) continue;
            auto const* node = sTaxiNodesStore.LookupEntry(dest);
            std::string name = node->name[0] ? node->name[0] : "Unknown destination";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, name + " - " + Money(Price(dest)), SENDER, PURCHASE_BASE + dest);
        }
    }
    SendGossipMenuFor(player, GOSSIP_TEXT, board->GetGUID());
}
}

// This script handles the board, while the spawner below attaches it to taxi masters.
class FlightPathDiscoveryBoard : public AllGameObjectScript
{
public:
    FlightPathDiscoveryBoard() : AllGameObjectScript("FlightPathDiscoveryBoard") { }
    void OnGameObjectRemoveWorld(GameObject* go) override
    {
        if (go->GetEntry() != FPD::BoardEntry()) return;
        std::lock_guard<std::mutex> guard(FPD::associationsLock);
        auto it = FPD::associations.find(go->GetGUID().GetRawValue());
        if (it != FPD::associations.end())
        {
            FPD::masterBoards.erase(it->second.GetRawValue());
            FPD::associations.erase(it);
        }
    }
    bool CanGameObjectGossipHello(Player* player, GameObject* go) override
    {
        if (!FPD::Enabled() || go->GetEntry() != FPD::BoardEntry()) return false;
        Creature* master = FPD::MasterFor(player, go);
        uint32 source = FPD::NodeFor(master, player);
        if (!source) { CloseGossipMenuFor(player); return true; }
        FPD::Menu(player, go, source);
        return true;
    }
    bool CanGameObjectGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action) override
    {
        if (go->GetEntry() != FPD::BoardEntry() || sender != FPD::SENDER) return false;
        Creature* master = FPD::MasterFor(player, go);
        uint32 source = FPD::NodeFor(master, player);
        if (!source) { CloseGossipMenuFor(player); return true; }
        if (action >= FPD::PURCHASE_BASE && action < FPD::PURCHASE_BASE + FPD::MAX_NODE)
        {
            uint32 destination = action - FPD::PURCHASE_BASE;
            if (!FPD::Eligible(player, source, destination)) { FPD::Menu(player, go, source); return true; }
            ClearGossipMenuFor(player);
            auto const* node = sTaxiNodesStore.LookupEntry(destination);
            std::string name = node->name[0] ? node->name[0] : "destination";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Confirm unlock: " + name + " for " + FPD::Money(FPD::Price(destination)), FPD::SENDER, FPD::CONFIRM_BASE + destination);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Cancel", FPD::SENDER, FPD::BACK);
            SendGossipMenuFor(player, FPD::GOSSIP_TEXT, go->GetGUID());
            return true;
        }
        if (action >= FPD::CONFIRM_BASE && action < FPD::CONFIRM_BASE + FPD::MAX_NODE)
        {
            uint32 destination = action - FPD::CONFIRM_BASE;
            uint32 price = FPD::Price(destination);
            if (price && FPD::Eligible(player, source, destination) && player->GetMoney() >= price)
            {
                player->ModifyMoney(-int32(price));
                player->m_taxi.SetTaximaskNode(destination);
            }
        }
        FPD::Menu(player, go, source);
        return true;
    }
};

class FlightPathDiscoverySpawner : public AllCreatureScript
{
public:
    FlightPathDiscoverySpawner() : AllCreatureScript("FlightPathDiscoverySpawner") { }
    void OnCreatureAddWorld(Creature* npc) override
    {
        if (!FPD::Enabled() || !npc || !FPD::SupportedMap(npc->GetMapId()) ||
            !npc->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_FLIGHTMASTER)) return;
        uint32 test = sConfigMgr->GetOption<uint32>("FlightPathDiscovery.TestCreatureEntry", 0);
        if (test && npc->GetEntry() != test) return;
        {
            std::lock_guard<std::mutex> guard(FPD::associationsLock);
            if (FPD::masterBoards.count(npc->GetGUID().GetRawValue())) return;
        }
        float dist = sConfigMgr->GetOption<float>("FlightPathDiscovery.BoardDistance", 2.5f);
        float a = npc->GetOrientation() + 1.57079632679f;
        GameObject* board = npc->SummonGameObject(FPD::BoardEntry(),
            npc->GetPositionX() + std::cos(a) * dist,
            npc->GetPositionY() + std::sin(a) * dist,
            npc->GetPositionZ(), npc->GetOrientation(),
            0.0f, 0.0f, std::sin(npc->GetOrientation() / 2),
            std::cos(npc->GetOrientation() / 2), 0, false);
        if (board)
        {
            std::lock_guard<std::mutex> guard(FPD::associationsLock);
            FPD::associations[board->GetGUID().GetRawValue()] = npc->GetGUID();
            FPD::masterBoards[npc->GetGUID().GetRawValue()] = board->GetGUID();
        }
    }

    void OnCreatureRemoveWorld(Creature* npc) override
    {
        if (!npc) return;
        ObjectGuid boardGuid;
        {
            std::lock_guard<std::mutex> guard(FPD::associationsLock);
            auto it = FPD::masterBoards.find(npc->GetGUID().GetRawValue());
            if (it == FPD::masterBoards.end()) return;
            boardGuid = it->second;
            FPD::associations.erase(boardGuid.GetRawValue());
            FPD::masterBoards.erase(it);
        }
        if (GameObject* board = npc->GetMap()->GetGameObject(boardGuid))
            board->Delete();
    }
};

// Load the configured flight path network during world startup,
// after AzerothCore has initialized its game data.
class FlightPathDiscoveryWorld : public WorldScript
{
public:
    FlightPathDiscoveryWorld() : WorldScript("FlightPathDiscoveryWorld") { }

    void OnStartup() override
    {
        FPD::LoadNetwork();
    }
};

void Addmod_flightpath_discoveryScripts()
{
    new FlightPathDiscoveryWorld();
    new FlightPathDiscoveryBoard();
    new FlightPathDiscoverySpawner();
}

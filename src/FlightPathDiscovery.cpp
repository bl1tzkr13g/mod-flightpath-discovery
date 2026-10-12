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
#include "DatabaseEnv.h"
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

// Price overrides are keyed by taxi-node ID, not NPC entry or area ID.
uint32 Price(uint32 destination)
{
    auto const* node = sTaxiNodesStore.LookupEntry(destination);
    if (!node || !SupportedMap(node->map_id)) return 0;

    std::string sql = "SELECT price_copper FROM mod_flightpath_discovery_prices WHERE taxi_node_id = " + std::to_string(destination);
    if (QueryResult result = WorldDatabase.Query(sql))
        return result->Fetch()[0].Get<uint32>();

    uint32 zone = sMapMgr->CreateBaseMap(node->map_id)->GetZoneId(1, node->x, node->y, node->z);
    auto const* area = sAreaTableStore.LookupEntry(zone);
    // Some zone records have area_level 0; never hide an approved route because of that.
    // Explicit node-level SQL prices take precedence and are recommended for consistency.
    int32 level = area ? area->area_level : 0;
    if (level < 1 && area && area->zone)
    {
        if (AreaTableEntry const* parent = sAreaTableStore.LookupEntry(area->zone))
            level = parent->area_level;
    }
    if (level < 1)
        level = sConfigMgr->GetOption<int32>("FlightPathDiscovery.UnknownZoneLevel", 1);
    if (level < 1) level = 1;
    if (level <= 10) return 1000;
    if (level <= 20) return 2500;
    if (level <= 30) return 7500;
    if (level <= 40) return 15000;
    if (level <= 50) return 30000;
    return 50000;
}

// The discovery network is deliberately separate from the underlying taxi graph.
// Only explicit source -> destination rows are purchasable.
bool ApprovedNeighbor(uint32 source, uint32 destination)
{
    std::string sql = "SELECT destination_node FROM mod_flightpath_discovery_routes WHERE source_node = "
        + std::to_string(source) + " AND destination_node = " + std::to_string(destination);
    return bool(WorldDatabase.Query(sql));
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
    std::string sql = "SELECT destination_node FROM mod_flightpath_discovery_routes WHERE source_node = " + std::to_string(source) + " ORDER BY destination_node";
    if (QueryResult result = WorldDatabase.Query(sql))
    {
        do
        {
            uint32 dest = result->Fetch()[0].Get<uint32>();
            if (dest >= MAX_NODE || !Eligible(player, source, dest)) continue;
            auto const* node = sTaxiNodesStore.LookupEntry(dest);
            std::string name = node->name[0] ? node->name[0] : "Unknown destination";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, name + " - " + Money(Price(dest)), SENDER, PURCHASE_BASE + dest);
        } while (result->NextRow());
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
        FPD::associations.erase(go->GetGUID().GetRawValue());
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
        if (!FPD::Enabled() || !npc || !FPD::SupportedMap(npc->GetMapId()) || !npc->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_FLIGHTMASTER)) return;
        uint32 test = sConfigMgr->GetOption<uint32>("FlightPathDiscovery.TestCreatureEntry", 0);
        if (test && npc->GetEntry() != test) return;
        float dist = sConfigMgr->GetOption<float>("FlightPathDiscovery.BoardDistance", 2.5f);
        float a = npc->GetOrientation() + 1.57079632679f;
        GameObject* board = npc->SummonGameObject(FPD::BoardEntry(), npc->GetPositionX() + std::cos(a) * dist,
            npc->GetPositionY() + std::sin(a) * dist, npc->GetPositionZ(), npc->GetOrientation(),
            0.0f, 0.0f, std::sin(npc->GetOrientation() / 2), std::cos(npc->GetOrientation() / 2), 43200, false);
        if (board)
        {
            std::lock_guard<std::mutex> guard(FPD::associationsLock);
            FPD::associations[board->GetGUID().GetRawValue()] = npc->GetGUID();
        }
    }
};

void Addmod_flightpath_discoveryScripts()
{
    new FlightPathDiscoveryBoard();
    new FlightPathDiscoverySpawner();
}

#include "botpch.h"
#include "../../playerbot.h"
#include "RpgAction.h"
#include "../../PlayerbotAIConfig.h"
#include "../values/PossibleRpgTargetsValue.h"
#include "GossipDef.h"

using namespace ai;

bool RpgAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "rpg target");
    if (!target)
        return false;

    if (sServerFacade.isMoving(bot))
        return false;

    if (bot->GetMapId() != target->GetMapId())
    {
        context->GetValue<Unit*>("rpg target")->Set(NULL);
        return false;
    }

    if (!sServerFacade.IsInFront(bot, target, sPlayerbotAIConfig.sightDistance, CAST_ANGLE_IN_FRONT) && !bot->IsTaxiFlying())
    {
        sServerFacade.SetFacingTo(bot, target, true);
        ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        return false;
    }

    if (!bot->GetNPCIfCanInteractWith(target->GetObjectGuid(), UNIT_NPC_FLAG_NONE))
        return false;

    if (target->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_FLIGHTMASTER))
    {
        taxi(target);
        return true;
    }

    vector<RpgElement> elements;
    elements.push_back(&RpgAction::cancel);
    elements.push_back(&RpgAction::emote);
    elements.push_back(&RpgAction::stay);
    elements.push_back(&RpgAction::work);

    RpgElement element = elements[urand(0, elements.size() - 1)];
    (this->*element)(target);
    return true;
}

void RpgAction::stay(Unit* unit)
{
    if (bot->PlayerTalkClass) bot->PlayerTalkClass->CloseGossip();
    ai->SetNextCheckDelay(sPlayerbotAIConfig.rpgDelay);
}

void RpgAction::work(Unit* unit)
{
    bot->HandleEmoteCommand(EMOTE_STATE_USESTANDING);
    ai->SetNextCheckDelay(sPlayerbotAIConfig.rpgDelay);
}

void RpgAction::emote(Unit* unit)
{
    vector<uint32> types;
    if (!urand(0, 20))
    {
        // expressions
        types.push_back(EMOTE_ONESHOT_BOW);
        types.push_back(EMOTE_ONESHOT_RUDE);
        types.push_back(EMOTE_ONESHOT_CRY);
        types.push_back(EMOTE_ONESHOT_LAUGH);
        types.push_back(EMOTE_ONESHOT_POINT);
        types.push_back(EMOTE_ONESHOT_CHEER);
        types.push_back(EMOTE_ONESHOT_SHY);
    }
    else
    {
        // talk
        types.push_back(EMOTE_ONESHOT_TALK);
        types.push_back(EMOTE_ONESHOT_EXCLAMATION);
        types.push_back(EMOTE_ONESHOT_QUESTION);
        if (unit->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_TRAINER) || unit->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
        {
            types.push_back(EMOTE_ONESHOT_SALUTE);
        }
    }
    uint32 type = types[urand(0, types.size() - 1)];

    ObjectGuid oldSelection = bot->GetSelectionGuid();

    bot->SetSelectionGuid(unit->GetObjectGuid());

    WorldPacket p1;
    p1 << unit->GetObjectGuid();
    bot->GetSession()->HandleGossipHelloOpcode(p1);

    bot->HandleEmoteCommand(type);
    unit->SetFacingTo(unit->GetAngle(bot));

    if (oldSelection)
        bot->SetSelectionGuid(oldSelection);

    ai->SetNextCheckDelay(sPlayerbotAIConfig.rpgDelay);
}

void RpgAction::cancel(Unit* unit)
{
    context->GetValue<Unit*>("rpg target")->Set(NULL);
}

void RpgAction::taxi(Unit* unit)
{
    uint32 curloc = sObjectMgr.GetNearestTaxiNode(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetMapId(), bot->GetTeam());

    vector<uint32> nodes;
    for (uint32 i = 0; i < sTaxiPathStore.GetNumRows(); ++i)
    {
        TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(i);
        if (entry && entry->from == curloc)
            nodes.push_back(i);
    }

    if (nodes.empty())
    {
        sLog.outDetail("Bot %s - No flight paths available", bot->GetName());
        return;
    }

    context->GetValue<Unit*>("rpg target")->Set(NULL);

    uint32 path = nodes[urand(0, nodes.size() - 1)];
    bot->m_taxi.SetTaximaskNode(path);
    uint32 money = bot->GetMoney();
    bot->ModifyMoney(money + 10000);

    TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(path);
    if (!entry)
        return;

    Creature* flightMaster = bot->GetNPCIfCanInteractWith(unit->GetObjectGuid(), UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!flightMaster)
    {
        sLog.outDetail("Bot %s cannot talk to flightmaster (%d location available)", bot->GetName(), nodes.size());
        return;
    }

    if (!bot->ActivateTaxiPathTo({ entry->from, entry->to }, flightMaster, 0))
    {
        sLog.outDetail("Bot %s cannot fly (%d location available)", bot->GetName(), nodes.size());
        return;
    }
    sLog.outDetail("Bot %s is flying to %u (%d location available)", bot->GetName(), path, nodes.size());
    bot->ModifyMoney(money);
}

bool RpgAction::isUseful()
{
    return context->GetValue<Unit*>("rpg target")->Get();
}

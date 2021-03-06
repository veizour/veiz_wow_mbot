#pragma once
#include "../Value.h"
#include "TargetValue.h"
#include "../../LootObjectStack.h"
#include "../../ServerFacade.h"

namespace ai
{
    class DistanceValue : public FloatCalculatedValue, public Qualified
	{
	public:
        DistanceValue(PlayerbotAI* ai) : FloatCalculatedValue(ai) {}

    public:
        float Calculate()
        {
            if (qualifier == "loot target")
            {
                LootObject loot = AI_VALUE(LootObject, qualifier);
                if (loot.IsEmpty())
                    return 0.0f;

                WorldObject* obj = loot.GetWorldObject(bot);
                if (!obj)
                    return 0.0f;

                return sServerFacade.GetDistance2d(ai->GetBot(), obj);
            }
            Unit* target = AI_VALUE(Unit*, qualifier);
            if (!target || !target->IsInWorld())
                return 0.0f;

            return sServerFacade.GetDistance2d(ai->GetBot(), target);
        }
    };
}

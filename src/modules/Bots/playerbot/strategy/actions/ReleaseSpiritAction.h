#pragma once

#include "../../ServerFacade.h"
#include "../Action.h"
#include "MovementActions.h"
#include "../values/LastMovementValue.h"

namespace ai
{
	class ReleaseSpiritAction : public Action {
	public:
		ReleaseSpiritAction(PlayerbotAI* ai) : Action(ai, "release") {}

    public:
        virtual bool Execute(Event event)
        {
            if (sServerFacade.IsAlive(bot) || bot->GetCorpse())
                return false;

            ai->ChangeStrategy("-follow,+stay", BOT_STATE_NON_COMBAT);

            bot->SetBotDeathTimer();
            bot->BuildPlayerRepop();

            bot->RepopAtGraveyard();
            return true;
        }
    };

}

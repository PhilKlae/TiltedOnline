#include <Games/References.h>

#include <Games/Skyrim/Forms/BGSAction.h>
#include <Games/Skyrim/Forms/TESIdleForm.h>

#include <Games/Fallout4/Forms/BGSAction.h>
#include <Games/Fallout4/Forms/TESIdleForm.h>

#include <ActionEvent.h>

#include <Games/Animation/ActorMediator.h>
#include <Games/Animation/TESActionData.h>

#include <Games/Skyrim/Misc/BSFixedString.h>
#include <Games/Fallout4/Misc/BSFixedString.h>

#include <World.h>

TP_THIS_FUNCTION(TPerformAction, uint8_t, ActorMediator, TESActionData* apAction);
static TPerformAction* RealPerformAction;

thread_local bool g_forceAnimation = false;

uint8_t TP_MAKE_THISCALL(HookPerformAction, ActorMediator, TESActionData* apAction)
{
    auto pActor = apAction->actor;
    const auto pExtension = pActor->GetExtension();

    if (!pExtension->IsRemote())
    {
        ActionEvent action;
        action.State1 = pActor->actorState.flags1;
        action.State2 = pActor->actorState.flags2;
        action.Type = apAction->unkInput | (apAction->someFlag ? 0x4 : 0);
        action.Tick = World::Get().GetTick();
        action.ActorId = pActor->formID;
        action.ActionId = apAction->action->formID;
        action.TargetId = apAction->target ? apAction->target->formID : 0;

        pActor->SaveAnimationVariables(action.Variables);

        const auto res = ThisCall(RealPerformAction, apThis, apAction);

        // This is a weird case where it gets spammed and doesn't do much, not sure if it still needs to be sent over the network
        if (apAction->someFlag == 1)
            return res;

        action.EventName = apAction->eventName.AsAscii();
        action.TargetEventName = apAction->targetEventName.AsAscii();
        action.IdleId = apAction->idleForm ? apAction->idleForm->formID : 0;

        // Save for later
        if (res)
            pExtension->LatestAnimation = action;

        pExtension->LatestVariables = action;

        World::Get().GetRunner().Trigger(action);

        return res;
    }

#if TP_FALLOUT4
    // Let the ActionInstantInitializeGraphToBaseState event go through
    if (apAction->action->formID == 0x5704c)
        return ThisCall(RealPerformAction, apThis, apAction);
#endif

    return 0;
}

ActorMediator* ActorMediator::Get() noexcept
{
    POINTER_FALLOUT4(ActorMediator*, s_actorMediator, 0x145AA4710 - 0x140000000);
    POINTER_SKYRIMSE(ActorMediator*, s_actorMediator, 0x142F271C0 - 0x140000000);

    return *(s_actorMediator.Get());
}

bool ActorMediator::PerformAction(TESActionData* apAction) noexcept
{
    if (apAction->actor->formID == 0x13482)
    {
        /*static Set<uint32_t> s_ids;

        spdlog::error("New frame");
        for(auto i = 0; i < action.Variables.size(); ++i)
        {
            auto& oldVars = pExtension->LatestVariables.Variables;
            auto& newVars = action.Variables;
            if(oldVars[i] != newVars[i] && s_ids.count(i) == 0)
            {
                //s_ids.insert(i);
                spdlog::info("Var {} changed from {} to {}", i, oldVars[i], newVars[i]);
            }
        }*/
        spdlog::info("Play animation name: {} with idle {:X} and target {:X} and unk {:X}", apAction->action->keyword.AsAscii(), (apAction->idleForm ? apAction->idleForm->formID : 0), (apAction->target ? apAction->target->formID : 0), apAction->unkInput);
    }

    const auto res = ThisCall(RealPerformAction, this, apAction);
    //const auto res = RePerformAction(apAction, aValue);

    if (res && apAction->actor->formID == 0x13482)
    {
        spdlog::info("Passed !");
    }

    return res != 0;
}

bool ActorMediator::ForceAction(TESActionData* apAction) noexcept
{
    TP_THIS_FUNCTION(TAnimationStep, uint8_t, ActorMediator, TESActionData*);
    using TApplyAnimationVariables = void* (void*, TESActionData*);

    POINTER_SKYRIMSE(TApplyAnimationVariables, ApplyAnimationVariables, 0x63D0F0);
    POINTER_SKYRIMSE(TAnimationStep, PerformComplexAction, 0x63B0F0);
    POINTER_SKYRIMSE(void*, qword_142F271B8, 0x142F271B8 - 0x140000000);

    POINTER_FALLOUT4(TAnimationStep, PerformComplexAction, 0x140E211A0 - 0x140000000);
    uint8_t result = 0;

    auto pActor = static_cast<Actor*>(apAction->actor);
    if (!pActor || pActor->animationGraphHolder.IsReady())
    {
        result = ThisCall(PerformComplexAction, this, apAction);

#if TP_SKYRIM64
        ApplyAnimationVariables(*qword_142F271B8.Get(), apAction);
#endif
    }

    return result;
}

ActionInput::ActionInput(uint32_t aParam1, Actor* apActor, BGSAction* apAction, TESObjectREFR* apTarget)
{
    // skip vtable as we never use this directly
    actor = apActor;
    target = apTarget;
    action = apAction;
    unkInput = aParam1;
}

void ActionInput::Release()
{
    actor.Release();
    target.Release();
}

ActionOutput::ActionOutput()
    : eventName("")
    , targetEventName("")
{
    // skip vtable as we never use this directly

    result = 0;
    targetIdleForm = nullptr;
    idleForm = nullptr;
    unk1 = 0;
}

void ActionOutput::Release()
{
    eventName.Release();
    targetEventName.Release();
}

BGSActionData::BGSActionData(uint32_t aParam1, Actor* apActor, BGSAction* apAction, TESObjectREFR* apTarget)
    : ActionInput(aParam1, apActor, apAction, apTarget)
{
    // skip vtable as we never use this directly
    someFlag = 0;
}

TESActionData::TESActionData(uint32_t aParam1, Actor* apActor, BGSAction* apAction, TESObjectREFR* apTarget)
    : BGSActionData(aParam1, apActor, apAction, apTarget)
{
    POINTER_FALLOUT4(void*, s_vtbl, 0x142C4A2A8 - 0x140000000);
    POINTER_SKYRIMSE(void*, s_vtbl, 0x141548198 - 0x140000000);

    someFlag = false;

    *reinterpret_cast<void**>(this) = s_vtbl.Get();
}

TESActionData::~TESActionData()
{
    ActionOutput::Release();
    ActionInput::Release();
}

static TiltedPhoques::Initializer s_animationHook([]()
{
    POINTER_FALLOUT4(TPerformAction, performAction, 0x140E20FB0 - 0x140000000);
    POINTER_SKYRIMSE(TPerformAction, performAction, 0x14063AF10 - 0x140000000);

    RealPerformAction = performAction.Get();

    TP_HOOK(&RealPerformAction, HookPerformAction);
});


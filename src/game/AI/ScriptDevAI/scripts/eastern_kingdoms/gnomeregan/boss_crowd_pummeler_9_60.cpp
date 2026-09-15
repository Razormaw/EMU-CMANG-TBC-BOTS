/* CMaNGOS TBC - Gnomeregan (map 90): Crowd Pummeler 9-60 */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "gnomeregan.h"

enum
{
    EMOTE_AGGRO               = -1090036,
    EMOTE_DEATH               = -1090037,

    SPELL_CROWD_PUMMEL        = 10887,    // VERIFICADO en tu cliente
};

struct boss_crowd_pummeler_9_60AI : public ScriptedAI
{
    boss_crowd_pummeler_9_60AI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiCrowdPummelTimer;

    void Reset() override
    {
        m_uiCrowdPummelTimer = urand(8000, 12000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiCrowdPummelTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_CROWD_PUMMEL) == CAST_OK)
                m_uiCrowdPummelTimer = urand(10000, 16000);
        }
        else
            m_uiCrowdPummelTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_crowd_pummeler_9_60(Creature* pCreature)
{
    return new boss_crowd_pummeler_9_60AI(pCreature);
}

void AddSC_boss_crowd_pummeler_9_60()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_crowd_pummeler_9_60";
    pNewScript->GetAI = &GetAI_boss_crowd_pummeler_9_60;
    pNewScript->RegisterSelf();
}


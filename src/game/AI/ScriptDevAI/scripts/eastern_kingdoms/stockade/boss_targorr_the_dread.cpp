/* CMaNGOS TBC - The Stockade (map 34): Targorr the Dread */

#include "AI/ScriptDevAI/include/sc_common.h"

enum
{
    SPELL_FEAR                    = 5782,
    SPELL_SHADOW_BOLT             = 1088,

    SAY_AGGRO                     = -1034000,
    SAY_SLAY                      = -1034001,
    SAY_DEATH                     = -1034002,
};

struct boss_targorr_the_dreadAI : public ScriptedAI
{
    boss_targorr_the_dreadAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiFearTimer;
    uint32 m_uiShadowBoltTimer;

    void Reset() override
    {
        m_uiFearTimer = urand(12000, 16000);
        m_uiShadowBoltTimer = urand(4000, 7000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiShadowBoltTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_BOLT) == CAST_OK)
                m_uiShadowBoltTimer = urand(4000, 7000);
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        if (m_uiFearTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FEAR) == CAST_OK)
                m_uiFearTimer = urand(12000, 16000);
        }
        else
            m_uiFearTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_targorr_the_dread(Creature* pCreature)
{
    return new boss_targorr_the_dreadAI(pCreature);
}

void AddSC_boss_targorr_the_dread()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_targorr_the_dread";
    pNewScript->GetAI = &GetAI_boss_targorr_the_dread;
    pNewScript->RegisterSelf();
}

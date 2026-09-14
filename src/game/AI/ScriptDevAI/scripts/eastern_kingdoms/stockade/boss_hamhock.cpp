/* CMaNGOS TBC - The Stockade (map 34): Hamhock */

#include "AI/ScriptDevAI/include/sc_common.h"

enum
{
    SPELL_CLEAVE                  = 845,
    SPELL_STOMP                   = 5589,
    SPELL_ENRAGE                  = 8599,

    SAY_AGGRO                     = -1034006,
    SAY_SLAY                      = -1034007,
    SAY_DEATH                     = -1034008,
    SAY_ENRAGE                    = -1034009,
};

struct boss_hamhockAI : public ScriptedAI
{
    boss_hamhockAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiCleaveTimer;
    uint32 m_uiStompTimer;
    bool m_bEnraged;

    void Reset() override
    {
        m_uiCleaveTimer = urand(6000, 9000);
        m_uiStompTimer = urand(12000, 17000);
        m_bEnraged = false;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
            {
                DoScriptText(SAY_ENRAGE, m_creature);
                m_bEnraged = true;
            }
        }

        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = urand(6000, 9000);
        }
        else
            m_uiCleaveTimer -= uiDiff;

        if (m_uiStompTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_STOMP) == CAST_OK)
                m_uiStompTimer = urand(12000, 17000);
        }
        else
            m_uiStompTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_hamhock(Creature* pCreature)
{
    return new boss_hamhockAI(pCreature);
}

void AddSC_boss_hamhock()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_hamhock";
    pNewScript->GetAI = &GetAI_boss_hamhock;
    pNewScript->RegisterSelf();
}

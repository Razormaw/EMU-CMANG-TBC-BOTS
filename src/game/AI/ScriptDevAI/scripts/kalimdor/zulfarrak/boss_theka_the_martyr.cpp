#include "AI/ScriptDevAI/include/sc_common.h"
#include "zulfarrak.h"

enum
{
    SAY_AGGRO                 = -1209008,
    SAY_TRANSFORM             = -1209009,
    SAY_SLAY                  = -1209010,
    SAY_DEATH                 = -1209011,

    SPELL_FEVERED_PLAGUE      =  8600,    // verificar con: .lookup spell Fevered Plague
    SPELL_THEKA_TRANSFORM     = 11089,    // VERIFICADO en tu cliente
};

struct boss_theka_the_martyrAI : public ScriptedAI
{
    boss_theka_the_martyrAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiPlagueTimer;
    bool m_bTransformed;

    void Reset() override
    {
        m_uiPlagueTimer = urand(8000, 14000);
        m_bTransformed = false;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bTransformed && m_creature->GetHealthPercent() < 25.0f)
        {
            DoScriptText(SAY_TRANSFORM, m_creature);
            DoCastSpellIfCan(m_creature, SPELL_THEKA_TRANSFORM);
            m_bTransformed = true;
        }

        if (m_uiPlagueTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FEVERED_PLAGUE) == CAST_OK)
                m_uiPlagueTimer = urand(10000, 16000);
        }
        else
            m_uiPlagueTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_theka_the_martyr(Creature* pCreature)
{
    return new boss_theka_the_martyrAI(pCreature);
}

void AddSC_boss_theka_the_martyr()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_theka_the_martyr";
    pNewScript->GetAI = &GetAI_boss_theka_the_martyr;
    pNewScript->RegisterSelf();
}
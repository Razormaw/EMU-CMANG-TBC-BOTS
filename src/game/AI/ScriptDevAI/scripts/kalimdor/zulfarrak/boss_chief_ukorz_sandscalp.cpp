#include "AI/ScriptDevAI/include/sc_common.h"
#include "zulfarrak.h"

enum
{
    SAY_AGGRO                 = -1209017,
    SAY_SLAY                  = -1209018,
    SAY_ENRAGE                = -1209019,
    SAY_DEATH                 = -1209020,

    SPELL_CLEAVE              = 11609,
    SPELL_WHIRLWIND           = 1680,
    SPELL_FRENZY              = 8599,
};

struct boss_chief_ukorz_sandscalpAI : public ScriptedAI
{
    boss_chief_ukorz_sandscalpAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiCleaveTimer;
    uint32 m_uiWhirlwindTimer;
    bool m_bFrenzied;

    void Reset() override
    {
        m_uiCleaveTimer = urand(6000, 9000);
        m_uiWhirlwindTimer = urand(14000, 20000);
        m_bFrenzied = false;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bFrenzied && m_creature->GetHealthPercent() < 30.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
            {
                DoScriptText(SAY_ENRAGE, m_creature);
                m_bFrenzied = true;
            }
        }

        if (m_uiCleaveTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK)
                m_uiCleaveTimer = urand(6000, 9000);
        }
        else
            m_uiCleaveTimer -= uiDiff;

        if (m_uiWhirlwindTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_WHIRLWIND) == CAST_OK)
                m_uiWhirlwindTimer = urand(14000, 20000);
        }
        else
            m_uiWhirlwindTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_chief_ukorz_sandscalp(Creature* pCreature)
{
    return new boss_chief_ukorz_sandscalpAI(pCreature);
}

void AddSC_boss_chief_ukorz_sandscalp()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_chief_ukorz_sandscalp";
    pNewScript->GetAI = &GetAI_boss_chief_ukorz_sandscalp;
    pNewScript->RegisterSelf();
}

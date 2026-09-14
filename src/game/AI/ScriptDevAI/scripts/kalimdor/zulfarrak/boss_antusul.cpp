#include "AI/ScriptDevAI/include/sc_common.h"
#include "zulfarrak.h"

enum
{
    SAY_AGGRO                 = -1209004,
    SAY_SUMMON                = -1209005,
    SAY_SLAY                  = -1209006,
    SAY_DEATH                 = -1209007,

    NPC_SERVANT_OF_ANTUSUL    = 8156,
    SPELL_HEALING_WAVE        = 12491,
    SPELL_EARTH_SHOCK         = 8045,
    SPELL_THUNDERCLAP         = 8198,
};

struct boss_antusulAI : public ScriptedAI
{
    boss_antusulAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiHealTimer;
    uint32 m_uiEarthShockTimer;
    uint32 m_uiThunderclapTimer;
    uint8 m_uiSummonCount;

    void Reset() override
    {
        m_uiHealTimer = urand(12000, 18000);
        m_uiEarthShockTimer = urand(8000, 14000);
        m_uiThunderclapTimer = urand(10000, 16000);
        m_uiSummonCount = 0;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void JustSummoned(Creature* pSummoned) override
    {
        if (Unit* pTarget = m_creature->GetVictim())
            pSummoned->AI()->AttackStart(pTarget);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        // Invoca sirvientes de las dunas dos veces durante la pelea
        if (m_uiSummonCount < 2 && m_creature->GetHealthPercent() < (60.0f - 30.0f * m_uiSummonCount))
        {
            DoScriptText(SAY_SUMMON, m_creature);
            float fX, fY, fZ;
            for (uint8 i = 0; i < 2; ++i)
            {
                m_creature->GetNearPoint(m_creature, fX, fY, fZ, 0.0f, 5.0f, frand(0.0f, M_PI_F * 2.0f));
                m_creature->SummonCreature(NPC_SERVANT_OF_ANTUSUL, fX, fY, fZ, m_creature->GetOrientation(), TEMPSPAWN_TIMED_OOC_DESPAWN, 60000);
            }
            ++m_uiSummonCount;
        }

        if (m_uiHealTimer < uiDiff)
        {
            if (Unit* pTarget = DoSelectLowestHpFriendly(40.0f))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_HEALING_WAVE) == CAST_OK)
                    m_uiHealTimer = urand(12000, 18000);
            }
        }
        else
            m_uiHealTimer -= uiDiff;

        if (m_uiEarthShockTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_EARTH_SHOCK) == CAST_OK)
                m_uiEarthShockTimer = urand(8000, 14000);
        }
        else
            m_uiEarthShockTimer -= uiDiff;

        if (m_uiThunderclapTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_THUNDERCLAP) == CAST_OK)
                m_uiThunderclapTimer = urand(10000, 16000);
        }
        else
            m_uiThunderclapTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_antusul(Creature* pCreature)
{
    return new boss_antusulAI(pCreature);
}

void AddSC_boss_antusul()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_antusul";
    pNewScript->GetAI = &GetAI_boss_antusul;
    pNewScript->RegisterSelf();
}

/* CMaNGOS TBC - Gnomeregan (map 90): Dark Iron Ambassador (rare) */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "gnomeregan.h"

enum
{
    SAY_AGGRO                 = -1090038,
    SAY_SUMMON                = -1090039,
    SAY_SLAY                  = -1090040,
    SAY_DEATH                 = -1090041,

    SPELL_SHADOW_BOLT         = 1106,     // Shadow Bolt rango 5 (jefe lvl 28)
    SPELL_IMMOLATE            = 2941,     // VERIFICADO en tu cliente (rank 4)
    SPELL_SUMMON_INFERNAL     = 12740,    // VERIFICADO en tu cliente
};

struct boss_dark_iron_ambassadorAI : public ScriptedAI
{
    boss_dark_iron_ambassadorAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiImmolateTimer;
    uint32 m_uiInfernalTimer;
    bool m_bSummoned;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(4000, 7000);
        m_uiImmolateTimer = urand(8000, 12000);
        m_uiInfernalTimer = 8000;
        m_bSummoned = false;
        m_attackDistance = 20.0f;
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

        // Invoca un infernal una única vez por combate
        if (!m_bSummoned)
        {
            if (m_uiInfernalTimer < uiDiff)
            {
                DoScriptText(SAY_SUMMON, m_creature);
                float fX, fY, fZ;
                m_creature->GetNearPoint(m_creature, fX, fY, fZ, 0.0f, 5.0f, frand(0.0f, M_PI_F * 2.0f));
                m_creature->SummonCreature(8559, fX, fY, fZ, m_creature->GetOrientation(), TEMPSPAWN_TIMED_OOC_DESPAWN, 60000); // Infernal
                m_bSummoned = true;
            }
            else
                m_uiInfernalTimer -= uiDiff;
        }

        if (m_uiShadowBoltTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_BOLT) == CAST_OK)
                m_uiShadowBoltTimer = urand(4000, 7000);
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        if (m_uiImmolateTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_IMMOLATE) == CAST_OK)
                m_uiImmolateTimer = urand(10000, 15000);
        }
        else
            m_uiImmolateTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_dark_iron_ambassador(Creature* pCreature)
{
    return new boss_dark_iron_ambassadorAI(pCreature);
}

void AddSC_boss_dark_iron_ambassador()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_dark_iron_ambassador";
    pNewScript->GetAI = &GetAI_boss_dark_iron_ambassador;
    pNewScript->RegisterSelf();
}


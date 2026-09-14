#include "AI/ScriptDevAI/include/sc_common.h"
#include "zulfarrak.h"

enum
{
    SAY_AGGRO                 = -1209014,
    SAY_SLAY                  = -1209015,
    SAY_DEATH                 = -1209016,

    SPELL_SHADOW_BOLT         = 12739,
    SPELL_PSYCHIC_SCREAM      = 10888,
    SPELL_RENEW               = 6078,
};

struct boss_sezzzizAI : public ScriptedAI
{
    boss_sezzzizAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiShadowBoltTimer;
    uint32 m_uiScreamTimer;
    uint32 m_uiRenewTimer;

    void Reset() override
    {
        m_uiShadowBoltTimer = urand(5000, 8000);
        m_uiScreamTimer = urand(14000, 20000);
        m_uiRenewTimer = urand(12000, 18000);
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
                m_uiShadowBoltTimer = urand(5000, 8000);
        }
        else
            m_uiShadowBoltTimer -= uiDiff;

        if (m_uiScreamTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_PSYCHIC_SCREAM) == CAST_OK)
                m_uiScreamTimer = urand(14000, 20000);
        }
        else
            m_uiScreamTimer -= uiDiff;

        if (m_uiRenewTimer < uiDiff)
        {
            if (Unit* pTarget = DoSelectLowestHpFriendly(40.0f))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_RENEW) == CAST_OK)
                    m_uiRenewTimer = urand(12000, 18000);
            }
        }
        else
            m_uiRenewTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_sezzziz(Creature* pCreature)
{
    return new boss_sezzzizAI(pCreature);
}

void AddSC_boss_sezzziz()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_sezzziz";
    pNewScript->GetAI = &GetAI_boss_sezzziz;
    pNewScript->RegisterSelf();
}

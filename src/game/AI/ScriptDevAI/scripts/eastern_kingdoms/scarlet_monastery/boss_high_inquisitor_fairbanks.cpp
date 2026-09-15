/* CMaNGOS TBC - Scarlet Monastery (map 189): High Inquisitor Fairbanks */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "scarlet_monastery.h"

enum
{
    SAY_AGGRO                 = -1189036,
    SAY_SLAY                  = -1189037,
    SAY_DEATH                 = -1189038,

    SPELL_SHADOW_WORD_PAIN    = 10892,
    SPELL_MIND_BLAST          = 8105,
    SPELL_RENEW               = 6078,
};

struct boss_high_inquisitor_fairbanksAI : public ScriptedAI
{
    boss_high_inquisitor_fairbanksAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiSWPTimer;
    uint32 m_uiMindBlastTimer;
    uint32 m_uiRenewTimer;

    void Reset() override
    {
        m_uiSWPTimer = urand(5000, 9000);
        m_uiMindBlastTimer = urand(8000, 12000);
        m_uiRenewTimer = urand(15000, 22000);
        m_attackDistance = 20.0f;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiSWPTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_WORD_PAIN) == CAST_OK)
                m_uiSWPTimer = urand(9000, 13000);
        }
        else
            m_uiSWPTimer -= uiDiff;

        if (m_uiMindBlastTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_MIND_BLAST) == CAST_OK)
                m_uiMindBlastTimer = urand(8000, 12000);
        }
        else
            m_uiMindBlastTimer -= uiDiff;

        // Se renueva a sí mismo cuando está herido
        if (m_creature->GetHealthPercent() < 60.0f)
        {
            if (m_uiRenewTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_RENEW) == CAST_OK)
                    m_uiRenewTimer = urand(18000, 25000);
            }
            else
                m_uiRenewTimer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_high_inquisitor_fairbanks(Creature* pCreature)
{
    return new boss_high_inquisitor_fairbanksAI(pCreature);
}

void AddSC_boss_high_inquisitor_fairbanks()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_high_inquisitor_fairbanks";
    pNewScript->GetAI = &GetAI_boss_high_inquisitor_fairbanks;
    pNewScript->RegisterSelf();
}


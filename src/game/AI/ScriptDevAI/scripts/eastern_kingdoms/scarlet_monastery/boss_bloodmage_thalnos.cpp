/* CMaNGOS TBC - Scarlet Monastery (map 189): Bloodmage Thalnos */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "scarlet_monastery.h"

enum
{
    SAY_AGGRO                 = -1189033,
    SAY_SLAY                  = -1189034,
    SAY_DEATH                 = -1189035,

    SPELL_FIREBALL            = 8400,
    SPELL_FIRE_NOVA           = 8499,
};

struct boss_bloodmage_thalnosAI : public ScriptedAI
{
    boss_bloodmage_thalnosAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiFireballTimer;
    uint32 m_uiFireNovaTimer;

    void Reset() override
    {
        m_uiFireballTimer = urand(4000, 7000);
        m_uiFireNovaTimer = urand(10000, 15000);
        m_attackDistance = 20.0f;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiFireballTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FIREBALL) == CAST_OK)
                m_uiFireballTimer = urand(4000, 7000);
        }
        else
            m_uiFireballTimer -= uiDiff;

        if (m_uiFireNovaTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_FIRE_NOVA) == CAST_OK)
                m_uiFireNovaTimer = urand(10000, 15000);
        }
        else
            m_uiFireNovaTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_bloodmage_thalnos(Creature* pCreature)
{
    return new boss_bloodmage_thalnosAI(pCreature);
}

void AddSC_boss_bloodmage_thalnos()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_bloodmage_thalnos";
    pNewScript->GetAI = &GetAI_boss_bloodmage_thalnos;
    pNewScript->RegisterSelf();
}


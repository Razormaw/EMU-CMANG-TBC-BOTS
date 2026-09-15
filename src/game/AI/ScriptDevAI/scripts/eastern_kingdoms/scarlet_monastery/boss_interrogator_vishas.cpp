/* CMaNGOS TBC - Scarlet Monastery (map 189): Interrogator Vishas */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "scarlet_monastery.h"

enum
{
    SAY_AGGRO                 = -1189030,
    SAY_SLAY                  = -1189031,
    SAY_DEATH                 = -1189032,

    SPELL_REND                = 11572,
    SPELL_HAMSTRING           = 1715,
};

struct boss_interrogator_vishasAI : public ScriptedAI
{
    boss_interrogator_vishasAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiRendTimer;
    uint32 m_uiHamstringTimer;

    void Reset() override
    {
        m_uiRendTimer = urand(6000, 10000);
        m_uiHamstringTimer = urand(9000, 14000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiRendTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK)
                m_uiRendTimer = urand(8000, 12000);
        }
        else
            m_uiRendTimer -= uiDiff;

        if (m_uiHamstringTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK)
                m_uiHamstringTimer = urand(9000, 14000);
        }
        else
            m_uiHamstringTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_interrogator_vishas(Creature* pCreature)
{
    return new boss_interrogator_vishasAI(pCreature);
}

void AddSC_boss_interrogator_vishas()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_interrogator_vishas";
    pNewScript->GetAI = &GetAI_boss_interrogator_vishas;
    pNewScript->RegisterSelf();
}

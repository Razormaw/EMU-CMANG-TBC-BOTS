#include "AI/ScriptDevAI/include/sc_common.h"

enum { SAY_AGGRO = -1043020, SAY_SLAY = -1043021, SAY_DEATH = -1043022, SPELL_REND = 11572, SPELL_HAMSTRING = 1715, NPC_LORD_COBRAHN = 3669 };

struct boss_lord_cobrahnAI : public ScriptedAI
{
    boss_lord_cobrahnAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer, m_uiHamstringTimer;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_uiHamstringTimer = urand(9000, 14000); }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiRendTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); }
        else m_uiRendTimer -= uiDiff;
        if (m_uiHamstringTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK) m_uiHamstringTimer = urand(10000, 15000); }
        else m_uiHamstringTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_lord_cobrahn(Creature* pCreature) { return new boss_lord_cobrahnAI(pCreature); }
void AddSC_boss_lord_cobrahn() { Script* pNewScript = new Script; pNewScript->Name = "boss_lord_cobrahn"; pNewScript->GetAI = &GetAI_boss_lord_cobrahn; pNewScript->RegisterSelf(); }


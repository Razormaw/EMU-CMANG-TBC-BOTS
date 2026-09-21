#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1043035, EMOTE_DEATH = -1043036, SPELL_REND = 11572, SPELL_KNOCK_AWAY = 10101, SPELL_ENRAGE = 8599, NPC_MUTANUS = 3654 };

struct boss_mutanus_the_devourerAI : public ScriptedAI
{
    boss_mutanus_the_devourerAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer, m_uiKnockTimer; bool m_bEnraged;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_uiKnockTimer = urand(12000, 16000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 25.0f)
        { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiRendTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); }
        else m_uiRendTimer -= uiDiff;
        if (m_uiKnockTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK) m_uiKnockTimer = urand(14000, 18000); }
        else m_uiKnockTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_mutanus_the_devourer(Creature* pCreature) { return new boss_mutanus_the_devourerAI(pCreature); }
void AddSC_boss_mutanus_the_devourer() { Script* pNewScript = new Script; pNewScript->Name = "boss_mutanus_the_devourer"; pNewScript->GetAI = &GetAI_boss_mutanus_the_devourer; pNewScript->RegisterSelf(); }


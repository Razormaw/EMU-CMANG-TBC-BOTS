#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1070005, EMOTE_DEATH = -1070006, SPELL_STOMP = 5589, SPELL_ENRAGE = 8599, NPC_IRONAYA = 7228 };

struct boss_ironayaAI : public ScriptedAI
{
    boss_ironayaAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiStompTimer; bool m_bEnraged;
    void Reset() override { m_uiStompTimer = urand(10000, 15000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f)
        { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiStompTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_STOMP) == CAST_OK) m_uiStompTimer = urand(12000, 18000); }
        else m_uiStompTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_ironaya(Creature* pCreature) { return new boss_ironayaAI(pCreature); }
void AddSC_boss_ironaya() { Script* pNewScript = new Script; pNewScript->Name = "boss_ironaya"; pNewScript->GetAI = &GetAI_boss_ironaya; pNewScript->RegisterSelf(); }


#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1070007, EMOTE_DEATH = -1070008, SPELL_KNOCK_AWAY = 10101, SPELL_STOMP = 5589, NPC_OBSIDIAN_SENTINEL = 7023 };

struct boss_obsidian_sentinelAI : public ScriptedAI
{
    boss_obsidian_sentinelAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiKnockTimer, m_uiStompTimer;
    void Reset() override { m_uiKnockTimer = urand(10000, 14000); m_uiStompTimer = urand(14000, 20000); }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiKnockTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK) m_uiKnockTimer = urand(12000, 16000); }
        else m_uiKnockTimer -= uiDiff;
        if (m_uiStompTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_STOMP) == CAST_OK) m_uiStompTimer = urand(16000, 22000); }
        else m_uiStompTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_obsidian_sentinel(Creature* pCreature) { return new boss_obsidian_sentinelAI(pCreature); }
void AddSC_boss_obsidian_sentinel() { Script* pNewScript = new Script; pNewScript->Name = "boss_obsidian_sentinel"; pNewScript->GetAI = &GetAI_boss_obsidian_sentinel; pNewScript->RegisterSelf(); }


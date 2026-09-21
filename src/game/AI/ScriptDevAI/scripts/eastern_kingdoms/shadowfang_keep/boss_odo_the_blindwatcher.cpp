#include "AI/ScriptDevAI/include/sc_common.h"
enum { EMOTE_AGGRO = -1033028, EMOTE_DEATH = -1033029, SPELL_SHADOW_BOLT = 12739, SPELL_FEAR = 5782, NPC_ODO = 4279 };
struct boss_odo_the_blindwatcherAI : public ScriptedAI
{
    boss_odo_the_blindwatcherAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiBoltTimer, m_uiFearTimer;
    void Reset() override { m_uiBoltTimer = urand(5000, 8000); m_uiFearTimer = urand(12000, 17000); m_attackDistance = 20.0f; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiBoltTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_BOLT) == CAST_OK) m_uiBoltTimer = urand(5000, 8000); } else m_uiBoltTimer -= uiDiff;
        if (m_uiFearTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FEAR) == CAST_OK) m_uiFearTimer = urand(14000, 19000); } else m_uiFearTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_odo_the_blindwatcher(Creature* pCreature) { return new boss_odo_the_blindwatcherAI(pCreature); }
void AddSC_boss_odo_the_blindwatcher() { Script* pNewScript = new Script; pNewScript->Name = "boss_odo_the_blindwatcher"; pNewScript->GetAI = &GetAI_boss_odo_the_blindwatcher; pNewScript->RegisterSelf(); }


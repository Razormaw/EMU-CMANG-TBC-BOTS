#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1033017, SAY_SLAY = -1033018, SAY_CURSE = -1033019, SPELL_SHADOW_BOLT = 12739, SPELL_FEAR = 5782, NPC_ARUGAL = 4275 };
struct boss_archmage_arugalAI : public ScriptedAI
{
    boss_archmage_arugalAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiBoltTimer, m_uiFearTimer;
    void Reset() override { m_uiBoltTimer = urand(4000, 7000); m_uiFearTimer = urand(14000, 19000); m_attackDistance = 20.0f; }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_CURSE, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiBoltTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_BOLT) == CAST_OK) m_uiBoltTimer = urand(4000, 7000); } else m_uiBoltTimer -= uiDiff;
        if (m_uiFearTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FEAR) == CAST_OK) m_uiFearTimer = urand(16000, 21000); } else m_uiFearTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_archmage_arugal(Creature* pCreature) { return new boss_archmage_arugalAI(pCreature); }
void AddSC_boss_archmage_arugal() { Script* pNewScript = new Script; pNewScript->Name = "boss_archmage_arugal"; pNewScript->GetAI = &GetAI_boss_archmage_arugal; pNewScript->RegisterSelf(); }


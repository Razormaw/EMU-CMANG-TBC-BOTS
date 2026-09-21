#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1043033, EMOTE_DEATH = -1043034, SPELL_TOXIC_VOLLEY = 21687, SPELL_RENEW = 6078, NPC_VERDAN = 5775 };

struct boss_verdan_the_everlivingAI : public ScriptedAI
{
    boss_verdan_the_everlivingAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiVolleyTimer, m_uiRenewTimer;
    void Reset() override { m_uiVolleyTimer = urand(8000, 12000); m_uiRenewTimer = urand(12000, 18000); }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiVolleyTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_TOXIC_VOLLEY) == CAST_OK) m_uiVolleyTimer = urand(10000, 15000); }
        else m_uiVolleyTimer -= uiDiff;
        if (m_creature->GetHealthPercent() < 70.0f && m_uiRenewTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_RENEW) == CAST_OK) m_uiRenewTimer = urand(15000, 22000); }
        else m_uiRenewTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_verdan_the_everliving(Creature* pCreature) { return new boss_verdan_the_everlivingAI(pCreature); }
void AddSC_boss_verdan_the_everliving() { Script* pNewScript = new Script; pNewScript->Name = "boss_verdan_the_everliving"; pNewScript->GetAI = &GetAI_boss_verdan_the_everliving; pNewScript->RegisterSelf(); }


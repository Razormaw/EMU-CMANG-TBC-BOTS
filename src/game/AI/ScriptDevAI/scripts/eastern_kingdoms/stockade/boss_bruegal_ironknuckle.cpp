/* CMaNGOS TBC - The Stockade (map 34): Bruegal Ironknuckle */

#include "AI/ScriptDevAI/include/sc_common.h"

enum
{
    SPELL_UPPERCUT                = 10966,
    SPELL_THUNDERCLAP             = 6343,
    SPELL_KNOCK_AWAY              = 10101,
    SPELL_ENRAGE                  = 8599,

    SAY_AGGRO                     = -1034017,
    SAY_SLAY                      = -1034018,
    SAY_DEATH                     = -1034019,
    SAY_ENRAGE                    = -1034020,
};

struct boss_bruegal_ironknuckleAI : public ScriptedAI
{
    boss_bruegal_ironknuckleAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiUppercutTimer;
    uint32 m_uiThunderclapTimer;
    uint32 m_uiKnockAwayTimer;
    bool m_bEnraged;

    void Reset() override
    {
        m_uiUppercutTimer = urand(9000, 13000);
        m_uiThunderclapTimer = urand(14000, 19000);
        m_uiKnockAwayTimer = urand(12000, 16000);
        m_bEnraged = false;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (!m_bEnraged && m_creature->GetHealthPercent() < 25.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
            {
                DoScriptText(SAY_ENRAGE, m_creature);
                m_bEnraged = true;
            }
        }

        if (m_uiUppercutTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_UPPERCUT) == CAST_OK)
                m_uiUppercutTimer = urand(9000, 13000);
        }
        else
            m_uiUppercutTimer -= uiDiff;

        if (m_uiThunderclapTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_THUNDERCLAP) == CAST_OK)
                m_uiThunderclapTimer = urand(14000, 19000);
        }
        else
            m_uiThunderclapTimer -= uiDiff;

        if (m_uiKnockAwayTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK)
                m_uiKnockAwayTimer = urand(12000, 16000);
        }
        else
            m_uiKnockAwayTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_bruegal_ironknuckle(Creature* pCreature)
{
    return new boss_bruegal_ironknuckleAI(pCreature);
}

void AddSC_boss_bruegal_ironknuckle()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_bruegal_ironknuckle";
    pNewScript->GetAI = &GetAI_boss_bruegal_ironknuckle;
    pNewScript->RegisterSelf();
}

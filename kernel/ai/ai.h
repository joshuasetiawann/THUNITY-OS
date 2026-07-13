/* THUOS - AI-native layer, kernel side (registry instance + `ai` shell command).
 *
 * Honest scope: this sets up and inspects the in-kernel AI model (services,
 * models, tasks, policy, audit). It performs NO inference and opens NO network
 * connection - THUOS has no TCP/IP stack or AI runtime yet. The bridge to a
 * local Thunity/Ollama server is DESIGN-ONLY. */
#ifndef THUOS_AI_H
#define THUOS_AI_H

void ai_init(void);                 /* register services/models + default local-only policy */
void ai_command(const char *args);  /* `ai [status|models|tasks|policy|bridge|help]` */

#endif /* THUOS_AI_H */

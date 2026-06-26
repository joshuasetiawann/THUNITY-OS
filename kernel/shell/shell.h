/* THUOS — interactive kernel shell. */
#ifndef THUOS_SHELL_H
#define THUOS_SHELL_H

/* Runs the THUOS shell loop forever (text-mode fallback). Never returns. */
void shell_run(void);

/* Event-loop interface: print the intro + prompt, then feed one key at a time
 * (used when the terminal is an app inside the graphical desktop). */
void shell_start(void);
void shell_feed_char(char c);

#endif /* THUOS_SHELL_H */

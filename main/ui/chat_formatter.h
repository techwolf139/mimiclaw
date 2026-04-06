#ifndef CHAT_FORMATTER_H
#define CHAT_FORMATTER_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Chat message types for rich formatting
typedef enum {
    CHAT_FORMAT_PLAIN = 0,
    CHAT_FORMAT_CODE,
    CHAT_FORMAT_WARNING,
    CHAT_FORMAT_SUCCESS,
    CHAT_FORMAT_ERROR,
    CHAT_FORMAT_SYSTEM
} chat_msg_format_t;

// Structured chat message with formatting
typedef struct {
    char role[16];
    char content[UI_CHAT_MSG_MAX_LEN];
    chat_msg_format_t format;
    uint32_t timestamp;
    bool read;
    bool persisted;  // Whether saved to flash
} formatted_chat_msg_t;

// Chat history management
typedef struct {
    formatted_chat_msg_t *messages;
    int capacity;
    int count;
    int display_start;  // For scrolling in UI
    bool has_persisted_data;
} chat_history_t;

/**
 * Initialize chat history with given capacity
 * @param capacity Maximum number of messages to store
 * @return Pointer to history or NULL on failure
 */
chat_history_t* chat_history_init(int capacity);

/**
 * Free chat history resources
 */
void chat_history_free(chat_history_t *history);

/**
 * Add a formatted message to history
 * @param history Chat history
 * @param role Message role (user/assistant/system)
 * @param content Message content
 * @param format Message formatting type
 * @return true on success, false on failure
 */
bool chat_history_add(chat_history_t *history, 
                     const char *role,
                     const char *content,
                     chat_msg_format_t format);

/**
 * Clear all messages from history
 */
void chat_history_clear(chat_history_t *history);

/**
 * Load chat history from SPIFFS
 * @param history Chat history to populate
 * @param filename File to load from
 * @return Number of messages loaded, or -1 on error
 */
int chat_history_load(chat_history_t *history, const char *filename);

/**
 * Save chat history to SPIFFS
 * @param history Chat history to save
 * @param filename File to save to
 * @return Number of messages saved, or -1 on error
 */
int chat_history_save(chat_history_t *history, const char *filename);

/**
 * Parse markdown or rich formatting from text
 * Determines the appropriate format type
 * @param text Input text
 * @param parsed Output buffer for parsed text (same size as input)
 * @param format Output format type
 * @return true if parsing succeeded
 */
bool chat_parse_formatting(const char *text, char *parsed, chat_msg_format_t *format);

#ifdef __cplusplus
}
#endif

#endif // CHAT_FORMATTER_H
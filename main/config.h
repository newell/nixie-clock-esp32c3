#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILENAME "/spiffs/config.json"

void config_init(void);
void write_default_config(const char *ssid, const char *password);
char* read_json_data();
void read_config_value(const char *key, char *value, size_t value_size);
void write_config_value(const char *key, const char *value);
void check_and_update_wifi_config(void);
void update_config_wifi(const char *ssid, const char *password, const char *prev_ssid, const char *prev_password);

#endif /* CONFIG_H */

/*
 * find a unused local address 
 * find a unused local sport
 */
extern const char *find_saddr(const char *addr);
extern const char *get_unused_sport(const char *saddr, const int proto);

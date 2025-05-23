#define HISTLEN 101
#define BUFLEN 1001

extern char history[HISTLEN][BUFLEN], (*hb)[BUFLEN], (*hc)[BUFLEN], (*ht)[BUFLEN];

void readhist(void);
void writehist(void);

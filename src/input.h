#define INPUT(name) struct context *name(struct context *context)

typedef INPUT((*Input));

enum {
	CTRLC = '\003',
	CTRLD,
	BACKSPACE = '\010',
	CLEAR = '\014',
	ESCAPE = '\033',
	FORWARD = 'f',
	BACKWARD = 'b',
	ARROW = '[',
	UP = 'A',
	DOWN,
	RIGHT,
	LEFT,
	DEL = '\177',
};

INPUT(stringinput);
INPUT(scriptinput);
INPUT(userinput);

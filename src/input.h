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

PIPELINE(stringinput);
PIPELINE(scriptinput);
PIPELINE(userinput);

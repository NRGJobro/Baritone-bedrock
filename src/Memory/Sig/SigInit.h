#pragma once

class SigInit {
public:
	static void addSigs();

private:
	static void initRender();
	static void initHooks();
	static void initRest();
};

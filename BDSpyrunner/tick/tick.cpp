#pragma once
#include <queue>
#include <functional>
#include "tick.h"
#include "../Ԥ����ͷ.h"


// Σ���������
static std::queue<std::function<void()>> todos;

void safeTick(std::function<void()> F) {
	todos.push(F);
}



// ����tick
THook(void,
	"?tick@Level@@UEAAXXZ",
	VA a1, VA a2, VA a3, VA a4) {
	original(a1, a2, a3, a4);
	size_t l = todos.size();
	if (l > 0) {
		for (int i = 0; i < l; i++) {
			todos.front()();
			todos.pop();
		}
	}
}

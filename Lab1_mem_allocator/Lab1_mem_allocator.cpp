#include <cstdio>
#include <cstdint>
#include <malloc.h>
#include <cassert>
#include "pch.h"
#include <iostream>
#include <cassert>

#define HEAP_SIZE 1024

static char heap[HEAP_SIZE];
static void* PTR;

// size_t неотрицательный целочисельный тип данных используемый для подсчёта или индексации массива
static size_t SIZE = 812;
#pragma pack(push, 1)
typedef struct header
{
	bool status; // 0 - свободен, 1 - используемый
	size_t previous_size;
	size_t size;
}header_t;
static int SIZE_H = sizeof(header_t);
void set_status(void* pointer, size_t status) // установление статуса в хедере
{
	((header_t*)pointer)->status = status;
}
bool get_status(void* pointer) // получение статуса из хедера
{
	return ((header_t*)pointer)->status;
}
void set_previous_size(void* pointer, size_t previous_size) // установка размера в предыдущем хедере
{
	((header_t*)pointer)->previous_size = previous_size;
}
size_t get_previous_size(void* pointer) // получение размера из предыдущего хедера
{
	return ((header_t*)pointer)->previous_size;
}
void set_size(void* pointer, size_t size) // установка размера
{
	((header_t*)pointer)->size = size;
}
size_t get_size(void* pointer) // получение размера
{

	return ((header_t*)pointer)->size;
}
void* get_next(void* pointer)
{
	// uint8_t - не отрицательный целочиселный 8битный тип данных
	if ((uint8_t*)pointer + get_size(pointer) + SIZE_H == (uint8_t*)PTR + SIZE + SIZE_H)
	{
		return nullptr;
	}
	return (uint8_t*)pointer + get_size(pointer) + SIZE_H;
}
void* get_previous(void* pointer)
{
	if (pointer == PTR)
	{
		return nullptr;
	}
	return (uint8_t*)pointer - get_previous_size(pointer) - SIZE_H;
}
void new_header(void* pointer, bool status, size_t previous_size, size_t size)
{
	header_t a;
	a.status = status;
	a.previous_size = previous_size;
	a.size = size;
	*((header_t*)pointer) = a;
}
void combine_headers(void* pointer1, void* pointer2) // создание одного хедера из двух
{
	assert(get_status(pointer1) == 0 && get_status(pointer2) == 0);
	set_size(pointer1, get_size(pointer1) + get_size(pointer2) + SIZE_H);
	// если следующий поинтер существует, то обновоить инфу про предыдущий
	if (get_next(pointer2) != nullptr)
	{
		set_previous_size(get_next(pointer2), get_size(pointer1));
	}
}
void* block(size_t size)
{
	void* pointer = heap + SIZE_H;
	new_header(pointer, false, 0, size);

	return pointer;
}
void* get_best(size_t size) //выбор лучшей ячейки памяти
{
	void* pointer = PTR;
	void* best = nullptr;
	while (pointer != nullptr)
	{
		if ((best == nullptr || get_size(best) > get_size(pointer)) && get_size(pointer) // если лучший пустой или имеет достаточно места
			>= size && get_status(pointer) == 0) // и ячейка не занята
		{
			best = pointer;
		}
		pointer = get_next(pointer); // переход к следующему
	}
	return best;
}

void* memory_allocator(size_t size)
{
	if (size % 4 != 0) //выравниваем по байтам
	{
		size = size - size % 4 + 4;
	}
	void* pointer = get_best(size);
	if (pointer == nullptr)
	{
		return pointer; // нет подходящей ячейки для аллокации
	}
	if (get_size(pointer) > size + SIZE_H)
	{
		new_header((uint8_t*)pointer + size + SIZE_H, false, size, get_size(pointer) - size - SIZE_H);
		set_size(pointer, size);
	}
	set_status(pointer, true);
	return (uint8_t*)pointer + SIZE_H;
}
void memory_free(void* pointer)
{
	pointer = (uint8_t*)pointer - SIZE_H;
	set_status(pointer, false);
	// если следующий блок свободен то сливаем их в 1 блок
	if (get_next(pointer) != nullptr && get_status(get_next(pointer)) == 0)
	{
		combine_headers(pointer, get_next(pointer));
	} // если предыдущий блок пуст, то сливаем их в 1 блок
	if (get_previous(pointer) != nullptr && get_status(get_previous(pointer)) == 0)
	{
		combine_headers(get_previous(pointer), pointer);
	}
}
void* memory_reallocator(void* pointer, size_t size)
{
	pointer = (uint8_t*)pointer - SIZE_H;
	if (size % 4 != 0)
	{
		size = size - size % 4 + 4;
	}
	if (get_size(pointer) == size)
	{
		return pointer;
	}
	if (get_size(pointer) > size) // если блок имеет больше памяти чем мы хотим установить
	{
		if (get_size(pointer) - size - SIZE_H >= 0) // новый размер должен быть больше размера хедера (достаточно места для создание блока данных)
		{
			new_header((uint8_t*)pointer + size + SIZE_H, false, size, get_size(pointer) - size -
				SIZE_H);
			set_size(pointer, size);
			if (get_next(get_next(pointer)) != nullptr &&
				get_status(get_next(get_next(pointer))) == 0)
			{
				combine_headers(get_next(pointer), get_next(get_next(pointer)));
			}
		}
		return pointer;
	}
	if (get_next(pointer) != nullptr && get_size(pointer) + get_size(get_next(pointer)) //если существует следующий блок и size_1 + size_2 >= size
		>= size)
	{
		new_header((uint8_t*)pointer + size + SIZE_H, false, size, get_size(get_next(pointer)) -
			(size - get_size(pointer))); //создаём новый блок в начале первого и теперь первый находится между новым и вторым
		set_size(pointer, size);
		return pointer;
	}
	if (get_next(pointer) != nullptr && get_status(get_next(pointer)) == 0 &&
		get_size(pointer) + get_size(get_next(pointer)) + SIZE_H >= size) // если второй блок свободен, просто добавляем его во второй блок
	{
		set_size(pointer, get_size(pointer) + get_size(get_next(pointer)) + SIZE_H);
		return pointer;
	}
	void* best = memory_allocator(size); //если новый размер больше чем block1 + block2
	if (best == nullptr)
	{
		return best;
	}
	memory_free((uint8_t*)pointer + SIZE_H);
	return best;
}
void memory_dump()
{
	void* pointer = PTR;
	size_t size_h = 0;
	size_t size_b = 0;
	printf(" ______________________________________________________________________\n");
	printf("% 15s | % 6s | % 7s | % 7s | % 15s | % 15s | \n", "address", "status", "pr_size", "size",
		"previous", "next");
	while (pointer != nullptr)
	{
		printf("% 15p | % 6d | % 7ld | % 7ld | % 15p | % 15p | \n", pointer, get_status(pointer),
			get_previous_size(pointer), get_size(pointer), get_previous(pointer),
			get_next(pointer));
		size_h = size_h + SIZE_H;
		size_b = size_b + get_size(pointer);
		pointer = get_next(pointer);
	}
	printf("----------------------------------------------------------------------\n");
	printf("headers: %ld\nblocks : %ld\nsummary : %ld\n\n\n", size_h, size_b, size_h +
		size_b);
}
int main()
{
	PTR = block(SIZE);
	void* x1 = memory_allocator(15);
	memory_dump();
	void* x2 = memory_allocator(2);
	memory_dump();
	void* x3 = memory_allocator(3);
	memory_dump();
	void* x4 = memory_allocator(80);
	memory_dump();
	memory_free(x2);
	memory_free(x3);
	memory_dump();
	void* x5 = memory_allocator(20);
	void* x6 = memory_allocator(20);
	void* x7 = memory_allocator(72);
	memory_dump();
	memory_reallocator(x7, 7);
	memory_dump();
	return 0;
}
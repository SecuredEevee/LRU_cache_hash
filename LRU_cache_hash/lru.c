#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#define  HASH_MAX  8
#define nullptr 0

#define GOLDEN_RATIO_PRIME_32 0x9e370001UL

struct hlist_node {
	struct hlist_node* next, ** pprev;
};

struct hlist_head {
	struct hlist_node* first;
};

void hlist_add_head(struct hlist_node* n, struct hlist_head* h)
{
	struct hlist_node* first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

void __hlist_del(struct hlist_node* n)
{
	struct hlist_node* next = n->next;
	struct hlist_node** pprev = n->pprev;

	*pprev = next;
	if (next)
		next->pprev = pprev;
}
void hlist_del(struct hlist_node* n)
{
	__hlist_del(n);
	n->next = nullptr;
	n->pprev = nullptr;
}

struct list_head {
	struct list_head* next, * prev;
};

void __list_add(struct list_head* new,
	struct list_head* prev,
	struct list_head* next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void list_add(struct list_head* new,
	struct list_head* head)
{
	__list_add(new, head, head->next);
}


void list_add_tail(struct list_head* new, struct list_head* head)
{
	__list_add(new, head->prev, head);
}

void __list_del(struct list_head* prev, struct list_head* next) {
	next->prev = prev;
	prev->next = next;
}

int __list_del_entry_valid(struct list_head* entry)
{
	return 1;
}
void __list_del_entry(struct list_head* entry)
{
	if (!__list_del_entry_valid(entry))
		return;

	__list_del(entry->prev, entry->next);
}


void list_move(struct list_head* list, struct list_head* head)
{
	__list_del_entry(list);
	list_add(list, head);
}

#define offsetof(TYPE, MEMBER)	((size_t)&((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

unsigned int hash_32(unsigned int val, unsigned int bits)
{
	unsigned int hash = val * GOLDEN_RATIO_PRIME_32;
	return hash >> (32 - bits);
}


typedef struct
{
	int pfn; // page frame number
	int data;
	struct hlist_node hnode;
	struct list_head list;
}page;


void display_htab(struct hlist_head* heads, struct list_head *head) {
	struct hlist_node* temp;
	puts("[HASH TABLE]");
	for (int i = 0; i < HASH_MAX; i++){
		printf("[%d]", i);
		page* p;

		for (temp = heads[i].first; temp; temp = temp->next)
		{
			p = container_of(temp, page, hnode);
			printf("<->[%2d]", p->pfn);
		}
		printf("\n");

	}
	puts("");
	puts("[LRU LIST]");
	printf("[lru]");
	struct list_head* tmp;
	page* p;
	for (tmp = head->next; tmp != head; tmp = tmp->next) {
		p = container_of(tmp, page, list);
		printf("<->[%2d]", p->pfn);
	}

}

page* hash_search(int pfn, struct hlist_head* heads) {

	struct hlist_node* temp;
	page* p;
	for (temp = heads->first; temp; temp = temp->next) {
		p = container_of(temp, page, hnode);
		if (p->pfn == pfn) {
			return p;
		}
	}
	return 0;
}


int main() {

	int cap = 0;
	struct hlist_head htab[HASH_MAX] = { 0, };
	page pages[40] = { 0, };
	struct list_head head = { &head, &head };
	struct list_head* last = &head;
	for (int i = 0; i < 40; ++i) {
		pages[i].pfn = i;
		pages[i].data = i + 10000; // # 2 
	}
	system("cls");
	// hlist_add_head(&pages[0].hnode, &htab[hash_32(0, 3)]);
	// hlist_add_head(&pages[1].hnode, &htab[0]);
	display_htab(htab, &head);

	getchar();
	srand(time(0));
	for (; ;) {
		system("cls");
		int rand_page_num = rand() % 40;
		printf("pfn = %d 접근\n", rand_page_num);
		int hash_key = hash_32(rand_page_num, 3);
		page *p = hash_search(rand_page_num, &htab[hash_key]);
		if (p) {
			list_move(&p->list, &head);// list move
			printf("기존 페이지 접근, data = %d\n", p->data);
		}
		else {
			if (cap > 19) { // page 꽉 차게 되면 
				struct list_head *tmp = &head;
				struct list_head* last = tmp->prev;
				page* p2 = container_of(last, page, list); // list_del
				// printf("last pfn: %d \n", p2->pfn);
				// printf("리스트전\n");
				__list_del(last->prev, tmp);
				// printf("해시전\n");
				hlist_del(&p2->hnode); 			   // hlist_del
				printf("page 꽉참, pfn = %d 퇴출\n", p2->pfn);
			}
			else {
				cap++;
			}
			hlist_add_head(&pages[rand_page_num].hnode, &htab[hash_key]); // hlist_add_head
			list_add(&pages[rand_page_num].list, &head);// list_add
			printf("새로운 page hash 추가, 현재 page 수 = %d\n", cap);
		}
		display_htab(htab, &head);
		getchar();
	}
	return 0;
}
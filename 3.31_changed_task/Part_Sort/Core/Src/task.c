/*
 * task.c
 *
 * Description: Competitor use this file for tasks
 */

/* Includes */

#include "task.h"

/* Variables */

const uint8_t lastDay[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const char* ptBy[3] = { "Name", "Cate", "Store" };
const char* ptSort[2] = { "Ascending", "Descending" };
const char* ptTime[6] = { "Year", "Month", "Day", "Hour", "Min", "Sec" };
const char* ptCate[4] = { "Res", "Cap", "IC", "ETC" };
const char* ptWorkCate[4] = { "SAVE", "USE", "FIND", "MKPT" };
const char* keyboard[4] = { "1234567890", "QWERTYUOIP", "ASDFGHJKL;", "ZXCVBNM,. " };
typedef enum { res, cap, ic, etc, }CATE;
typedef enum { basic, left, right, up, down, }JOY;
typedef struct { uint8_t x, y; }POS;
typedef struct { uint8_t year, month, day, hour, min, sec; }TIME;

struct Part{
	char* name;
	uint8_t cate, pos;
	uint16_t store, max;
	POS temp;
};
struct PtLog{
	char title[30];
	char content1[22];
	char content2[22];
};
typedef struct{
	uint8_t sortNameF;
	uint8_t sortStoreF;
	uint8_t sortTypeF;
	uint8_t sortByprior[3];
	uint8_t sortCateprior[4];
}SORT_STATE;
struct Part pt[36], ptfirst;
struct PtLog ptLog[6];

uint8_t firF = 1, buzM = 0, ModeF = 0, sel = 0, start_check, findC, ptionC, ledM, use_num = 1;
uint8_t sortBypriorF[3], sortCatepriorF[4];
uint16_t buzC = 0, adcV[2], ledC=0, ledC, cnt;
char* input_name = NULL;
char* find_name = NULL;
JOY joy_result = basic;
CATE cate_sel = res;
TIME time = { 23, 3, 31, 0, 0, 0 };
POS temp, min, max, ptionS, ptionE;
SORT_STATE standard;

/* Functions */

__STATIC_INLINE uint8_t calxy(uint8_t x, uint8_t y) { return x + ((5 - y) * 6); };
__STATIC_INLINE uint8_t curxy(void) { return temp.x + ((5 - temp.y) * 6); }
__STATIC_INLINE void init_value(void) { firF = sel = ModeF = joy_result = temp.x = temp.y = 0; }

__STATIC_INLINE void get_time(void){
	DS3231_get_date(&time.day, &time.month, &time.year);
	DS3231_get_time(&time.sec, &time.min, &time.hour);
}

__STATIC_INLINE void set_time(void){
	DS3231_set_date(time.day, time.month, time.year);
	DS3231_set_time(time.sec, time.min, time.hour);
}

void ssd1306_putsXY(uint8_t x, uint8_t y, char* str, uint8_t color){
	SSD1306_GotoXY(x * 6, y * 8);
	SSD1306_Puts(str, &Font_6x8, color);
}

void array_puts(POS* pos, char* title, char** array, uint8_t color, uint8_t num){
	SSD1306_Fill(0);
	SSD1306_DrawFilledRectangle(0, 0, 127, 7, 1);
	ssd1306_putsXY(0, 0, title, 0);
	for(uint8_t i = 0 ; i < num ; i++) ssd1306_putsXY(pos[i].x, pos[i].y, array[i], color);
}

void get_adc(void){
	static uint32_t frev_tick;
	uint32_t now_tick = HAL_GetTick();

	if(now_tick - frev_tick > 150){
		frev_tick = now_tick;
		if(JOY_U) joy_result = up;
		if(JOY_D) joy_result = down;
		if(JOY_L) joy_result = left;
		if(JOY_R) joy_result = right;
	}
}

void get_sel(uint8_t* sel, uint8_t max, uint8_t min, uint8_t state){
	switch(state){
	case 0:
		if(joy_result == up) { if(*sel < max) *sel += 1; }
		if(joy_result == down) { if(*sel > min) *sel -= 1; }
		break;
	case 1:
		if(joy_result == right) { if(*sel < max) *sel += 1; }
		if(joy_result == left) { if(*sel > min) *sel -= 1; }
		break;
	case 2:
		if(joy_result == down) { if(*sel < max) *sel += 1; }
		if(joy_result == up) { if(*sel > min) *sel -= 1; }
		break;
	}
}

uint8_t read_sw(void){
	static uint8_t oldSW;
	if(JOY_P && !oldSW) { oldSW = 1; return 1; }
	if(!JOY_P && oldSW) { oldSW = 0; }
	return 0;
}

void led_display(struct Part* a, uint8_t i){
	switch(a->cate){
	case 1: led_color(i, 4, 1, 0); break;
	case 2: led_color(i, 1, 0, 4); break;
	case 3: led_color(i, 4, 4, 0); break;
	case 4: led_color(i, 0, 4, 4); break;
	}
}

char* input_string(char* str){
	uint8_t keyX = 0, keyY = 0;
	uint8_t limitX = 9, limitY = 3;
	uint8_t input_sel = 0;
	char* input_keyboard = (char*)calloc(sizeof(char), 11);
	while(1){
		POS pos[3] = {{0, 1}, {1 + input_sel, 2}, {1, 1}};
		char* array[3] = { ">", "^", input_keyboard };
		array_puts(pos, str, array, 1, sizeof(pos) / 2);

		for(uint8_t i = 0 ; i < 4 ; i++)
			for(uint8_t j = 0 ; j < 10 ; j++){
				SSD1306_GotoXY(14 + j * 10, (4 * 8) + (i * 8));
				SSD1306_Putc(keyboard[i][j], &Font_6x8, !(keyX == j && keyY == i));
			}
		ssd1306_putsXY(19, 4, "&", !(keyX == 10 && keyY == 0));
		ssd1306_putsXY(19, 6, "*", !(keyX == 10 && keyY == 2));
		SSD1306_UpdateScreen();

		if(keyY == 0 || keyY == 2) limitX = 10;
		else limitX = 9;

		get_adc();
		if(joy_result == left) { keyX = keyX > 0 ? keyX - 1 : limitX; }
		if(joy_result == right) { keyX = keyX < limitX ? keyX + 1 : 0; }
		if(joy_result == up){
			if(keyX == 10) { keyY = keyY == 0 ? 2 : 0; }
			else { keyY = keyY > 0 ? keyY - 1 : limitY; }
		}
		if(joy_result == down){
			if(keyX == 10) { keyY = keyY == 0 ? 2 : 0; }
			else { keyY = keyY < limitY ? keyY + 1 : 0; }
		}
		joy_result = basic;
		if(read_sw()){
			if(keyX < 10) input_keyboard[input_sel++] = keyboard[keyY][keyX];
			else{
				if(keyY == 0) input_keyboard[input_sel > 0 ? --input_sel : 0] = '\0';
				if(keyY == 2) break;
			}
		}
		if(input_sel >= 10) break;
	}
	firF = 0;
	return input_keyboard;
}

void logShift(uint8_t workCate){
	static uint8_t work_cnt;
	for(uint8_t i = work_cnt ; i > 0 ; i--) ptLog[i] = ptLog[i - 1];
	if(work_cnt < 5) work_cnt++;

	get_time();
	sprintf(ptLog[0].title, "%02d.%02d.%02d/%02d:%02d/%s", time.year, time.month, time.day, time.hour, time.min, ptWorkCate[workCate]);
	switch(workCate){
	case 0:
		sprintf(ptLog[0].content1, "%s/%s", pt[curxy()].name, ptCate[pt[curxy()].cate - 1]);
		sprintf(ptLog[0].content2, "%dpcs (%d,%d)", pt[curxy()].store, pt[curxy()].temp.x + 1, pt[curxy()].temp.y + 1);
		break;
	case 1:
		sprintf(ptLog[0].content1, "%s/%s", pt[curxy()].name, ptCate[pt[curxy()].cate - 1]);
		sprintf(ptLog[0].content2, "%dpcs (%d,%d)", use_num, pt[curxy()].temp.x + 1, pt[curxy()].temp.y + 1);
		break;
	case 2:
		sprintf(ptLog[0].content1, "Sear:%s", find_name);
		sprintf(ptLog[0].content2, "Num of Find:%d", findC);
		break;
	case 3:
		sprintf(ptLog[0].content1, "S(%d,%d) E(%d,%d)", ptionS.x + 1, ptionS.y + 1, ptionE.x + 1, ptionE.y + 1);
		sprintf(ptLog[0].content2, "Size of ption:%d", ptionC);
		break;
	}
}

uint8_t start(void){
	for(uint8_t i = 0 ; i < 36 ; i++)
		if(i / 6 == 0 || i / 6 == 3) led_color(i, 4, 0, 0);
		else if(i / 6 == 1 || i / 6 == 4) led_color(i, 0, 4, 0);
		else if(i / 6 == 2 || i / 6 == 5) led_color(i, 0, 0, 4);
	led_update();

	for(uint8_t i = 0 ; i < 9 ; i++){
		SSD1306_GotoXY(43, 24 + i);
		SSD1306_Puts("Drawer", &Font_6x8, 0);

		SSD1306_GotoXY(39, 24);
		SSD1306_Puts(" Parts ", &Font_6x8, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(200);
	}
	HAL_Delay(1000);
	SSD1306_Clear();
	led_clear();
	led_update();
	if(!eepReadData(0)) return 1;
	else return 0;
}

void time_settting(void){
	while(!read_sw()){
		get_adc();
		get_sel(&sel, 5, 0, 1);
		if(sel == 0) get_sel(&time.year, 99, 0, 0);
		if(sel == 1) get_sel(&time.month, 12, 1, 0);
		if(sel == 2) get_sel(&time.day, lastDay[time.month - 1], 1, 0);
		if(sel == 3) get_sel(&time.hour, 23, 0, 0);
		if(sel == 4) get_sel(&time.min, 59, 0, 0);
		if(sel == 5) get_sel(&time.sec, 59, 0, 0);
		if(time.day > lastDay[time.month - 1]) time.day = lastDay[time.month - 1];
		joy_result = basic;

		POS pos[2] = {{0, 2}, {0, 4}};
		char bf[20];
		sprintf(bf, "%s=%02d", ptTime[sel], sel == 0 ? 2000 + time.year : sel == 1 ? time.month : sel == 2 ? time.day : sel == 3 ? time.hour : sel == 4 ? time.min : time.sec);
		char* array[2] = { "RTC Time setting.", bf };
		array_puts(pos, "#Time set", array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();
	}
	set_time();
	eepWriteData(0, 1);
}

void main_menu(void){
	static uint8_t check;
	if(!firF){
		firF = 1;
		led_clear();
		for(uint8_t i = 0 ; i < 36 ; i++) led_display(&pt[i], i);
		led_update();
		for(uint8_t i = 0 ; i < 36 ; i++)
			if(pt[i].cate != 0) { check = 1; break; }
			else check = 0;
	}
	get_time();
	POS pos[9] = {{0, 2 + sel}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {11, 3}, {13, 4}};
	char bf[2][20];
	sprintf(bf[0], "%04d.%02d.%02d", 2000 + time.year, time.month, time.day);
	sprintf(bf[1], "%02d:%02d:%02d", time.hour, time.min, time.sec);
	char* array[9] = { ">", "Part save", "Part use", "Part find", "Partition", "Part log", "Part sort", bf[0], bf[1] };
	array_puts(pos, "#Menu", array, 1, sizeof(pos) / 2);
	SSD1306_UpdateScreen();

	get_adc();
	get_sel(&sel, 5, 0, 2);
	joy_result = basic;

	if(read_sw()){
		if(sel == 0) { led_clear(); led_update(); start_check++; }
		ModeF = sel + 1;
		firF = 0;
		if(!check && sel != 0) { buzM = 1; ModeF = 0; }
		else sel = 0;
	}
}

void position_set(void){
	POS new = temp;
	while(!read_sw()){
		POS pos = {1, 6};
		char bf[20];
		sprintf(bf, "Position (%d,%d)", temp.x + 1, temp.y + 1);
		ssd1306_putsXY(pos.x, pos.y, bf, 0);
		SSD1306_UpdateScreen();

		led_clear();
		for(uint8_t i = 0 ; i < 36 ; i++) pt[i].cate != 0 ? led_color(i, 4, 0, 0) : led_color(i, 0, 4, 0);
		led_color(curxy(), 4, 4, 4);
		led_update();

		if(cnt > 150){
			cnt = 0;
			if(JOY_U) do{ new.y++; }while(led_cmp(calxy(new.x, new.y), 4, 0, 0));
			if(JOY_D) do{ new.y--; }while(led_cmp(calxy(new.x, new.y), 4, 0, 0));
			if(JOY_L) do{ new.x--; }while(led_cmp(calxy(new.x, new.y), 4, 0, 0));
			if(JOY_R) do{ new.x++; }while(led_cmp(calxy(new.x, new.y), 4, 0, 0));
		}
		if(new.x > 5) new.x = temp.x;
		if(new.y > 5) new.y = temp.y;
		temp = new;
	}
	firF = 0;
}

void save_mode(void){
	static uint8_t store_num = 1, store_max;
	store_max = cate_sel == res ? 200 : cate_sel == cap ? 100 : cate_sel == ic ? 50 : 10;
	if(!firF){
		firF = 1;
		POS pos[7] = {{0, 3 + sel}, {0, 1}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}};
		char bf[4][20];
		sprintf(bf[0], "Cate:%s", ptCate[cate_sel]);
		sprintf(bf[1], "Name:%s", input_name[0] > 0 ? input_name : "(NONE)");
		sprintf(bf[2], "Store: %d/%d", store_num, store_max);
		sprintf(bf[3], "Position (%d,%d)", temp.x + 1, temp.y + 1);
		char* array[7] = { ">", "Pls input information", bf[0], bf[1], bf[2], bf[3], "Enter" };
		array_puts(pos, "#Save", array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();
	}
	get_adc();
	get_sel(&sel, 4, 0, 2);
	if(sel == 0) get_sel(&cate_sel, etc, res, 1);
	if(sel == 2) get_sel(&store_num, store_max, 1, 1);
	if(joy_result != basic) { joy_result = basic; firF = 0; }
	if(read_sw()){
		if(sel == 1) input_name = input_string("input Name");
		if(sel == 3) position_set();
		if(sel == 4){
			if(input_name[0] > 0){
				pt[curxy()].name = input_name;
				pt[curxy()].cate = cate_sel + 1;
				pt[curxy()].max = store_max;
				pt[curxy()].store = store_num;
				pt[curxy()].pos = start_check;
				pt[curxy()].temp = temp;

				logShift(0);

				if(start_check == 1) ptfirst = pt[curxy()];

				input_name = NULL;
				cate_sel = res;
				store_num = 1;
				init_value();
			}
			else buzM = 2;
		}
	}
}

void part_refill(void){
	uint8_t refill_num = 0;
	while(!read_sw()){
		POS pos[2] = {{0, 1}, {0, 3}};
		char bf[20];
		sprintf(bf, "IN:%d/%d", refill_num, pt[curxy()].max);
		char* array[2] = { "Part has all used.", bf };
		array_puts(pos, "#refill", array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();

		get_adc();
		get_sel(&refill_num, pt[curxy()].max, 0, 0);
		joy_result = basic;
	}
	if(refill_num > 0) { pt[curxy()].store += refill_num; logShift(0); }
	else { free(pt[curxy()].name); memset(&pt[curxy()], 0, sizeof(struct Part)); }
}

void part_use(void){
	use_num = 1;

	led_clear();
	for(uint8_t i = 0 ; i < 36 ; i++) led_display(&pt[i], i);
	led_color(curxy(), 4, 4, 4);
	led_update();

	while(!read_sw()){
		POS pos[2] = {{0, 1}, {0, 3}};
		char bf[20];
		sprintf(bf, "(%d/%d)", use_num, pt[curxy()].store);
		char* array[2] = { "How many use?", bf };
		array_puts(pos, "#Use", array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();

		get_adc();
		get_sel(&use_num, pt[curxy()].store, 1, 0);
		joy_result = basic;
	}
	logShift(1);
	pt[curxy()].store -= use_num;
	if(pt[curxy()].store == 0) part_refill();
}

void use_mode(void){
	if(!firF){
		firF = 1;
		POS pos[4] = {{0, 1}, {0, 3}, {0, 4}, {0, 5}};
		char bf[3][20];
		sprintf(bf[0], pt[curxy()].cate != 0 ? "Cate:%s" : "(Empty)", ptCate[pt[curxy()].cate - 1]);
		sprintf(bf[1], "Name:%s", pt[curxy()].cate != 0 ? pt[curxy()].name : "");
		sprintf(bf[2], "Store:%d", pt[curxy()].store);
		char* array[4] = { "Select part", bf[0], bf[1], bf[2] };
		array_puts(pos, "#Use", array, 1, pt[curxy()].cate != 0 ? sizeof(pos) / 2 : 2);
		SSD1306_UpdateScreen();

		led_clear();
		for(uint8_t i = 0 ; i < 36 ; i++) led_display(&pt[i], i);
		led_color(curxy(), 4, 4, 4);
		led_update();
	}
	if(cnt > 150){
		cnt = 0;
		if(JOY_U) temp.y = temp.y < 5 ? temp.y + 1 : temp.y;
		if(JOY_D) temp.y = temp.y > 0 ? temp.y - 1 : temp.y;
		if(JOY_L) temp.x = temp.x > 0 ? temp.x - 1 : temp.x;
		if(JOY_R) temp.x = temp.x < 5 ? temp.x + 1 : temp.x;
		if(JOY_U || JOY_D || JOY_L || JOY_R) firF = 0;
	}
	if(read_sw()){
		if(pt[curxy()].cate != 0) { part_use(); init_value(); }
		else buzM = 2;
	}
}

void find_result(char* str){
	uint8_t find_num = 0;
	for(uint8_t i = 0 ; i < 36 ; i++) if(!strcmp(pt[i].name, str)) find_num++;
	struct Part* find_part = (struct Part*)calloc(sizeof(struct Part), find_num);
	findC = 0;
	for(uint8_t i = 0 ; i < start_check ; i++)
		for(uint8_t j = 0 ; j < 36 ; j++)
			if(!strcmp(pt[j].name, str) && pt[j].pos - 1 == i) find_part[findC++] = pt[j];
	while(!read_sw()){
		if(findC > 0){
			get_adc();
			get_sel(&sel, findC - 1, 0, 2);
			joy_result = basic;

			led_clear();
			for(uint8_t i = 0 ; i < 36 ; i++) led_display(&pt[i], i);
			if(ledC < 500) led_display(&find_part[sel], calxy(find_part[sel].temp.x, find_part[sel].temp.y));
			else led_color(calxy(find_part[sel].temp.x,  find_part[sel].temp.y), 0, 0, 0);
			led_update();
		}
		POS pos[2 + findC];
		char bf[findC][30];
		pos[0].x = 0, pos[0].y = 1;
		pos[1].x = 0, pos[1].y = 2 + sel;
		for(uint8_t i = 0 ; i < findC ; i++){
			sprintf(bf[i], "%s(%s/%d,%d)", find_part[i].name, ptCate[find_part[sel].cate - 1], find_part[i].temp.x + 1, find_part[i].temp.y + 1);
			pos[2 + i].x = 1, pos[2 + i].y = 2 + i;
		}
		char* array[2 + findC];
		array[0] = findC > 0 ? "Find some parts!" : "Not found..";
		array[1] = ">";
		for(uint8_t i = 0 ; i < findC ; i++) array[2 + i] = bf[i];
		array_puts(pos, "#Find result", array, 1, findC > 0 ? 2 + findC : 1);
		SSD1306_UpdateScreen();
	}
	logShift(2);
	if(findC > 0) { temp = find_part[sel].temp; part_use(); }
}

void find_mode(void){
	static uint8_t screenState;
	if(!firF){
		firF = 1;
		POS pos = {3, 4};
		char bf[20];
		sprintf(bf, find_name[0] > 0 ? find_name : "Input find name");
		char* array = { bf };
		array_puts(&pos, "#Find", &array, 1, sizeof(pos) / 2);
		SSD1306_DrawRectangle(pos.x * 6 - 2, pos.y * 8 - 3, strlen("Input find name") * 6 + 3, 12, 1);
		SSD1306_UpdateScreen();
	}
	if(read_sw()){
		if(!screenState) { find_name = input_string("#input part name"); firF = 0; }
		else{
			{ ledM = 1; find_result(find_name); ledM = 0; }
			{ free(find_name); find_name = NULL; }
			init_value();
		}
		screenState = find_name[0] > 0;
	}
}

void partition_save(void){
	struct Part spart;
	memset(&spart, 0, sizeof(struct Part));
	ptionC = 0;
	for(uint8_t i = min.y ; i <= max.y ; i++)
		for(uint8_t j = min.x ; j <= max.x ; j++){
			if(pt[calxy(j, i)].cate != 0) spart = pt[calxy(j, i)];
			ptionC++;
		}
	if(spart.cate == 0) spart = ptfirst;

	for(uint8_t i = min.y ; i <= max.y ; i++)
		for(uint8_t j = min.x ; j <= max.x ; j++){
			if(calxy(j, i) != calxy(spart.temp.x, spart.temp.y)){
				pt[calxy(j,  i)] = spart;
				pt[calxy(j,  i)].pos = ++start_check;
				pt[calxy(j,  i)].temp.x = j;
				pt[calxy(j,  i)].temp.y = i;
			}
			pt[calxy(j,  i)].max *= ptionC;
		}
	while(!read_sw()){
		POS pos = {0, 3};
		char bf[20];
		sprintf(bf, "Total number:%d", ptionC);
		char* array = { bf };
		array_puts(&pos, "#Partition", &array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();

		led_clear();
		if(ledC < 500)
			for(uint8_t i = min.y ; i <= max.y ; i++)
				for(uint8_t j = min.x ; j <= max.x ; j++)
					led_color(calxy(j,  i), 0, 4, 0);
		else led_clear();
		led_update();
	}
	logShift(3);
}

void partition_mode(void){
	static uint8_t screenState;
	if(!firF){
		firF = 1;
		POS pos[3] = {{0, 1}, {0, 3}, {0, 4}};
		char bf[2][20];
		sprintf(bf[0], !screenState ? "Start (%d,%d)" : "End (%d,%d)", temp.x + 1, temp.y + 1);
		sprintf(bf[1], pt[curxy()].cate != 0 ? "%s(%s/%d/%d)" : "(NONE)", pt[curxy()].name, ptCate[pt[curxy()].cate - 1], pt[curxy()].temp.x + 1, pt[curxy()].temp.y + 1);
		char* array[3] = { !screenState ? "Select start point" : "Select end point", bf[0], bf[1] };
		array_puts(pos, "#Partition", array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();

		led_clear();
		for(uint8_t i = 0 ; i < 36 ; i++) led_display(&pt[i], i);
		led_color(curxy(), 4, 4, 4);
		led_update();
	}
	if(cnt > 150){
		cnt = 0;
		if(JOY_U) temp.y = temp.y < 5 ? temp.y + 1 : temp.y;
		if(JOY_D) temp.y = temp.y > 0 ? temp.y - 1 : temp.y;
		if(JOY_L) temp.x = temp.x > 0 ? temp.x - 1 : temp.x;
		if(JOY_R) temp.x = temp.x < 5 ? temp.x + 1 : temp.x;
		if(JOY_U || JOY_D || JOY_L || JOY_R) firF = 0;
	}
	if(read_sw()){
		if(!screenState) { ptionS = temp; firF = 0; }
		else{
			ptionE = temp;

			min.x = ptionS.x > ptionE.x ? ptionE.x : ptionS.x;
			min.y = ptionS.y > ptionE.y ? ptionE.y : ptionS.y;

			max.x = ptionS.x > ptionE.x ? ptionS.x : ptionE.x;
			max.y = ptionS.y > ptionE.y ? ptionS.y : ptionE.y;

			{ ledM = 1; partition_save(); ledM = 0; }
			init_value();
		}
		screenState = !screenState;
	}
}

void log_detail(uint8_t i){
	POS pos[3] = {{0, 2}, {0, 4}, {0, 5}};
	char* array[3] = { ptLog[i].title, ptLog[i].content1, ptLog[i].content2 };
	array_puts(pos, "#Log detail", array, 1, sizeof(pos) / 2);
	SSD1306_UpdateScreen();
	while(!read_sw());
}

void log_mode(void){
	static uint8_t log_num;
	if(!firF){
		firF = 1;
		POS pos[7] = {{0, 2 + sel}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}};
		char bf[6][30];
		log_num = 0;
		for(uint8_t i = 0 ; i < 6 ; i++){
			if(!strlen(ptLog[i].title)) break;
			sprintf(bf[i], "%s", ptLog[i].title);
			log_num++;
		}
		char* array[7] = { ">", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5] };
		array_puts(pos, "#Log", array, 1, log_num + 1);
		SSD1306_UpdateScreen();
	}
	get_adc();
	get_sel(&sel, log_num - 1, 0, 2);
	if(joy_result != basic) { if(joy_result == left) init_value(); joy_result = basic; firF = 0; }
	if(read_sw()) { log_detail(sel); init_value(); }
}

void by_set(void){
	uint8_t by_sel = 0, num = 0;
	memset(sortBypriorF, 0, 3);
	memset(standard.sortByprior, 0, 3);
	while(1){
		char bf[30];
		sprintf(bf, "By:%-5s>%-5s>%-5s", num == 0 ? ptBy[by_sel] : standard.sortByprior[0] ? ptBy[standard.sortByprior[0] - 1] : " ",
				num == 1 ? ptBy[by_sel] : standard.sortByprior[1] ? ptBy[standard.sortByprior[1] - 1] : " ",
				num == 2 ? ptBy[by_sel] : standard.sortByprior[2] ? ptBy[standard.sortByprior[2] - 1] : " ");
		ssd1306_putsXY(1, 2, bf, 0);
		SSD1306_UpdateScreen();

		if(num < 2){
			get_adc();
			if(joy_result == up) { do{ by_sel = (by_sel + 1) < 3 ? by_sel + 1 : 0; }while(sortBypriorF[by_sel]); }
			if(joy_result == down) { do{ by_sel = by_sel > 0 ? by_sel - 1 : 2; }while(sortBypriorF[by_sel]); }
			joy_result = basic;
		}

		if(read_sw()){
			sortBypriorF[by_sel] = 1;
			standard.sortByprior[num++] = by_sel + 1;
			if(num < 3) { by_sel = 0; while(sortBypriorF[by_sel]) by_sel++; }
			else { firF = 0; break; }
		}
	}
}

void cate_set(void){
	uint8_t cate_sel = 0, num = 0;
	memset(sortCatepriorF, 0, 4);
	memset(standard.sortCateprior, 0, 4);
	while(1){
		char bf[30];
		sprintf(bf, "Cate:%-3s>%-3s>%-3s>%-3s", num == 0 ? ptCate[cate_sel] : standard.sortCateprior[0] ? ptCate[standard.sortCateprior[0] - 1] : " ",
				num == 1 ? ptCate[cate_sel] : standard.sortCateprior[1] ? ptCate[standard.sortCateprior[1] - 1] : " ",
						num == 2 ? ptCate[cate_sel] : standard.sortCateprior[2] ? ptCate[standard.sortCateprior[2] - 1] : " ",
								num == 3 ? ptCate[cate_sel] : standard.sortCateprior[3] ? ptCate[standard.sortCateprior[3] - 1] : " ");
		ssd1306_putsXY(1, 3, bf, 0);
		SSD1306_UpdateScreen();

		if(num < 3){
			get_adc();
			if(joy_result == up) { do{ cate_sel = (cate_sel + 1) < 4 ? cate_sel + 1 : 0; }while(sortCatepriorF[cate_sel]); }
			if(joy_result == down) { do{ cate_sel = cate_sel > 0 ? cate_sel - 1 : 3; }while(sortCatepriorF[cate_sel]); }
			joy_result = basic;
		}

		if(read_sw()){
			sortCatepriorF[cate_sel] = 1;
			standard.sortCateprior[num++] = cate_sel + 1;
			if(num < 4) { cate_sel = 0; while(sortCatepriorF[cate_sel]) cate_sel++; }
			else { firF = 0; break; }
		}
	}
}

volatile void part_swap(struct Part* a, struct Part* b){
	struct Part temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

volatile int8_t name_compare(struct Part* a, struct Part* b){
	int8_t res = 0;
	res = strcmp(a->name, b->name);
	if(!standard.sortNameF) res *= -1;
	return res;
}

volatile int8_t cate_compare(struct Part* a, struct Part* b){
	int8_t res = 0;
	uint8_t a_pos = 0, b_pos = 0;
	while(a->cate != standard.sortCateprior[a_pos]) a_pos++;
	while(b->cate != standard.sortCateprior[b_pos]) b_pos++;
	res = b_pos - a_pos;
	return res;
}

volatile int8_t store_compare(struct Part* a, struct Part* b){
	int8_t res = 0;
	res = a->store - b->store;
	if(!standard.sortStoreF) res *= -1;
	return res;
}

volatile int8_t (*compare_item[3])(struct Part* a, struct Part* b) = { name_compare, cate_compare, store_compare };

volatile void part_sort(void){
	int8_t res = 0, swapF = 0;

	for(uint8_t i = 0 ; i < start_check ; i++)
		for(temp.y = 0 ; temp.y < 6 ; temp.y++)
			for(temp.x = 0 ; temp.x < 6 ; temp.x++)
				if(pt[curxy()].cate != 0){
					POS cur = temp;
					do{ if(cur.x % 6 == 5) cur.y++; cur.x++; }while(pt[calxy(cur.x % 6, cur.y)].cate == 0);
					if(cur.x >= 36) break;

					res = compare_item[standard.sortByprior[0] - 1](&pt[curxy()], &pt[calxy(cur.x % 6, cur.y)]);
					if(res == 0){
						res = compare_item[standard.sortByprior[1] - 1](&pt[curxy()], &pt[calxy(cur.x % 6, cur.y)]);
						if(res == 0){
							res = compare_item[standard.sortByprior[2] - 1](&pt[curxy()], &pt[calxy(cur.x % 6, cur.y)]);
							if(res < 0) swapF = 1;
						}
						else if(res < 0) swapF = 1;
					}
					else if(res < 0) swapF = 1;
					if(swapF) { part_swap(&pt[curxy()], &pt[calxy(cur.x % 6, cur.y)]); swapF = 0; }
				}

	POS cur = {0, 0};
	for(temp.y = 0 ; temp.y < 6 ; temp.y++)
		for(temp.x = 0 ; temp.x < 6;  temp.x++)
			if(standard.sortTypeF){
				if(pt[curxy()].cate != 0){
					part_swap(&pt[curxy()], &pt[calxy(cur.x % 6, cur.y)]);
					cur.x++;
					cur.y = cur.x / 6;
				}
			}
			else temp.x = temp.y = 6;

	for(temp.y = 0 ; temp.y < 6 ; temp.y++)
		for(temp.x = 0 ; temp.x < 6;  temp.x++)
			if(pt[curxy()].cate != 0) pt[curxy()].temp = temp;
			else memset(&pt[curxy()], 0, sizeof(struct Part));
}

volatile void sort_mode(void){
	static uint8_t byF, cateF;
	if(!firF){
		firF = 1;
		POS pos[8] = {{0, 2 + sel}, {0, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}};
		char bf[5][30];
		sprintf(bf[0], !byF ? "By:(NONE)" : "By:%-5s>%-5s>%-5s", ptBy[standard.sortByprior[0] - 1], ptBy[standard.sortByprior[1] - 1], ptBy[standard.sortByprior[2] - 1]);
		sprintf(bf[1], !cateF ? "Cate:(NONE)" : "Cate:%-3s>%-3s>%-3s>%-3s", ptCate[standard.sortCateprior[0] - 1], ptCate[standard.sortCateprior[1] - 1], ptCate[standard.sortCateprior[2] - 1], ptCate[standard.sortCateprior[3] - 1]);
		sprintf(bf[2], "Name:%s", ptSort[standard.sortNameF]);
		sprintf(bf[3], "Store:%s", ptSort[standard.sortStoreF]);
		sprintf(bf[4], "Option:Type %d", standard.sortTypeF + 1);
		char* array[8] = { ">", "Sort Option Setting.", bf[0], bf[1], bf[2], bf[3], bf[4], "Enter" };
		array_puts(pos, "#Sort", array, 1, sizeof(pos) / 2);
		SSD1306_UpdateScreen();
	}
	get_adc();
	get_sel(&sel, 5, 0, 2);
	if(sel == 2) get_sel(&standard.sortNameF, 1, 0, 1);
	if(sel == 3) get_sel(&standard.sortStoreF, 1, 0, 1);
	if(sel == 4) get_sel(&standard.sortTypeF, 1, 0, 1);
	if(joy_result != basic) { joy_result = basic; firF = 0; }
	if(read_sw()){
		if(sel == 0) { by_set(); byF = 1; }
		if(sel == 1) { cate_set(); cateF = 1; }
		if(sel == 5) { part_sort(); byF = cateF = 0; memset(&standard, 0, sizeof(SORT_STATE)); init_value(); }
	}
}

void (*play_task[7])(void) = { main_menu, save_mode, use_mode, find_mode, partition_mode, log_mode, sort_mode };

/* Tasks */

void PSDrawers_Initialized(void)
{
	SSD1306_Init();

	HAL_TIM_Base_Start_IT(&htim2);

	/* Write user code here */
	if(start()) time_settting();
	init_value();

	pt[curxy()].name = "A";
	pt[curxy()].cate = cap + 1;
	pt[curxy()].store = 20;
	pt[curxy()].temp = temp;

	temp.x++;
	temp.y++;
	start_check++;

	pt[curxy()].name = "B";
	pt[curxy()].cate = cap + 1;
	pt[curxy()].store = 20;
	pt[curxy()].temp = temp;

	temp.x++;
	temp.y++;
	start_check++;

	pt[curxy()].name = "C";
	pt[curxy()].cate = etc + 1;
	pt[curxy()].store = 5;
	pt[curxy()].temp = temp;

	temp.x++;
	temp.y++;
	start_check++;

	pt[curxy()].name = "D";
	pt[curxy()].cate = ic + 1;
	pt[curxy()].store = 10;
	pt[curxy()].temp = temp;

	temp.x++;
	temp.y++;
	start_check++;

	pt[curxy()].name = "E";
	pt[curxy()].cate = res + 1;
	pt[curxy()].store = 25;
	pt[curxy()].temp = temp;

	temp.x++;
	temp.y++;
	start_check++;
}

void PSDrawers_Main(void)
{
	while(1)
	{
		/* Write user code here */
		play_task[ModeF]();
	}
}

/* Callbacks */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM2)
	{
		HAL_ADC_Start(&hadc);
		HAL_ADC_PollForConversion(&hadc, 10);
		adcV[0] = HAL_ADC_GetValue(&hadc);
		HAL_ADC_PollForConversion(&hadc, 10);
		adcV[1] = HAL_ADC_GetValue(&hadc);
		HAL_ADC_Stop(&hadc);
		cnt++;
		if(buzM) buzC++;
		if(buzM == 1){
			if(buzC < 500) BUZ(1);
			else { buzM = buzC = 0; BUZ(0); }
		}
		if(buzM == 2){
			if(buzC < 25) BUZ(1);
			else if(buzC < 50) BUZ(0);
			else if(buzC < 75) BUZ(1);
			else { buzM = buzC = 0; BUZ(0); }
		}
		if(ledM) ledC = ledC < 1000 ? ledC + 1 : 0;
		else ledC = 0;
	}
}

section .text


; Принимает код возврата и завершает текущий процесс
exit:
	mov rax, 60  						; номер системного вызова (60 - exit) 
	syscall 							; 
	ret

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
	xor rax, rax						; cnt = 0
.loop:
	cmp byte [rdi + rax], 0				; при получении нуль-терминатора 
	je .end								; выходим из цикла
	inc rax								; cnt++
	jmp .loop							; 
.end:
	ret									; return с rax = cnt

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:	
	push rdi							; сохраняем адрес строки
	call string_length					; rax = длина строки
	pop rsi								; rsi = адрес строки
	mov rdx, rax						; rdi = rax = длина строки
	mov rax, 1							; номер системного вызова(1 - вывод)
	mov rdi, 1							; файловый дескриптор(1 - stdout)
	syscall								; системный вызов для вывода строки
	ret								

; Принимает код символа и выводит его в stdout
print_char:
	push rdi							; пушим char в стек 
	mov rdi, rsp						; записываем адрес char в rdi для вызова print_string 
	call print_string					; 
	pop rdi								; корректируем rsp
	ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
	mov rdi, 0xA						; записываем '\n' в rdi
	call print_char						; выводим '\n'
	ret 

; Выводит беззнаковое 8-байтовое число в десятичном формате
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:
	mov r9, 10							; в r9 храним 10 для "разбора" числа(пусть x) на части
	mov rax, rdi						; x записываем в rax, чтобы потом rax % 10 нам давал его последнюю цифру
	mov rdi, rsp						; аллоцируем буффер в стеке для текстового представления числа
	dec rdi 							; при этом в rdi сохраняем rsp до корреции
	push 0 								; записываем нуль терминатор для корректного исполнения print_string
	sub rsp, 16 						; корректируем rsp

.loop: 									
	xor rdx, rdx 						; обнуляем rdx 
	div r9 								; x = x / 10; rdx = x % 10 
	add rdx, '0' 						; rdx теперь содержит ASCII код последней цифры x(до деления на 10)
	dec rdi 							; корректируем rdi, теперь он указывает на свободный элемент буффера  
	mov [rdi], dl  						; помещаем последнюю цифру x в буффер
	cmp rax, 0							; проверка на то, закончились ли цифры:
	je .end 							; if(x == 0) goto .end
	jmp .loop 							; else goto .loop

.end:
	call print_string 					; выводим буффер
	add rsp, 24 						; корректируем rsp
	ret 					

; Выводит знаковое 8-байтовое число в десятичном формате
print_int: 
	cmp rdi, 0 							; if(x < 0) goto .minus 
	jl .minus 							; 
.plus:
	call print_uint 					; если x >= 0 выводим его как uint
	jmp .end 							; 
.minus:  
	push rdi 							;
	mov rdi, '-' 						; если x < 0 то сначала выводим '-'
	call print_char 					;
	pop rdi 							; 
	neg rdi 							; а затем выводим (-x) как uint
	call print_uint 					;
.end:
	ret

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
	xor rax, rax  						; str_eq = 0
	xor rcx, rcx 						; i = 0
	xor r9, r9 							; buf1 = 0
	xor r10, r10 						; buf2 = 0
.loop:
	mov r9b, byte [rdi + rcx] 			; buf1 = str1[i]
	mov r10b, byte [rsi + rcx] 			; buf2 = str2[i]
	cmp r9b, r10b 						; if(buf1 != buf2) goto .neq
	jne .neq 							;
	cmp r9b, 0 							; если встречаем нуль-терминатор, то строки равны(тк были равны до него, включая его) 
	je .eq 								; 
	inc rcx 							; иначе i++; goto .loop
	jmp .loop
.eq:
	mov rax, 1 							; если строки равны, то str_eq = 1
	ret 								; возвращаем str_eq 
.neq:
	mov rax, 0 							; если строки не равны, то str_eq = 0
	ret 								; возвращаем str_eq 

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
	xor rax, rax 						; номер системного вызова(0 - ввод)
	xor rdi, rdi 						; файловый дескриптор (0 - stdin)
	push 0 								; создаем буффер в стеке
	mov rsi, rsp 						; адрес для записи(rsp) помещаем в rsi для syscall
	mov rdx, 1 							; количество байт ввода = 1
	syscall 							; системный вызов на чтение символа в буффер
	pop rax 							; забираем код символа из буффера в rax и возвращаем
	ret

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор

read_word:
	xor r9, r9 							; symb_cnt = 0
	mov r10, rdi 						; buf_adr = rdi
.loop:
	push rsi 							;
	push rdi							;
	call read_char 						; считываем символ с сохранением rsi, rdi
	pop rdi 							;
	pop rsi 							;
	test r9, r9 						; if(symb_cnt == 0) goto .tab_check
	je .tab_check 						; 
.sym:
	cmp r9, rsi 						; проверка на переполнение буффера
	jae .err 							; при переполнении goto .err
	cmp rax, 0x20 						; проверка на всевозможные пробельные символы
	je .add_null 						; если таковой встречен, то слово закончилось
	cmp rax, 0x9 						; и мы делаем goto .add_null
	je .add_null 						; 
	cmp rax, 0xA 						;
	je .add_null 						;
	mov [rdi + r9], rax 				; иначе записываем символ в буффер
	test rax, rax 						; если это нуль-терминатор, 
	je .end 							; то делаем goto .end
	inc r9 								; иначе symb_cnt++
	jmp .loop 							; и делаем goto .loop

.tab_check:
	cmp rax, 0x20 						; проверка на всевозможные пробельные символы
	je .loop 							; если таковой встречен, то пропускаем его
	cmp rax, 0x9 						; и делаем goto .loop
	je .loop 							;
	cmp rax, 0xA 						;
	je .loop 							;
	jmp .sym 							; иначе goto .sym

.add_null:
	xor rax, rax 						; в этой ветке мы добавляем нуль-терминатор
	mov [rdi + r9], rax 				; к слову, дописывая его в буфер 
	jmp .end 							; далее goto .end

.err:
	xor rax, rax 						; при ошибке возвращаем 0 в rax
	ret 								;
.end: 
	mov rax, r10 						; при успехе rax = buf_adr
	mov rdx, r9 						; и rdx = symb_cnt
	ret 								;

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint:
	mov r8, 10  						; r8 = 10 для умножений на 10
	xor rax, rax 						; rax = 0, в нем мы будем хранить число
	xor rcx, rcx 						; len = 0
.loop:
	movzx r9, byte [rdi + rcx] 			; r9 = buf[len], то есть r9 = код потенциальной цифры
	cmp r9b, '0' 						; если в r9 не код цифры
	jb .end 							; то завершаем функцию
	cmp r9b, '9' 						; 
	ja .end 							;
	xor rdx, rdx 						; иначе умножаем rax на 10
	mul r8								; 
	sub r9b, '0' 						; в r9 теперь сама цифра, а не ее код
	add rax, r9 						; добавляем r9 к rax
	inc rcx								; len++
	jmp .loop 							; goto .loop
.end:
	mov rdx, rcx 						; rdx = len, причем rdx = 0 если число не прочитано
	ret

; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был)
; rdx = 0 если число прочитать не удалось
parse_int:
	mov r8, 10 							; r8 = 10 для умножения на 10
	xor rax, rax 						; rax = 0, будет аккумулировать число
	xor rcx, rcx 						; len = 0
	push 1 								; is_positive = [rsp] = 1
.loop:
	movzx r9, byte [rdi + rcx] 			; r9 = buff[len]
	cmp r9b, '-' 						; if(r9 == '-') goto .minus
	je .minus 							;
	cmp r9b, '0' 						; если r9 не содержит число, то 
	jb .check 							; goto .check
	cmp r9b, '9' 						;
	ja .check 							;
	xor rdx, rdx 						; rax = rax * 10
	mul r8 								;
	sub r9b, '0' 						; r9 теперь содержит цифру, а не ее код
	add rax, r9 						; rax = rax + r9
.continue:
	inc rcx 							; len++
	jmp .loop 							; 

.minus:
	mov r10, 0 							; is_positive = 0
	mov [rsp], r10 						; 
	jmp .continue 						; 

.check:
	pop r10 							; if(!is_positive) goto .neg
	cmp r10, 0 							; 
	je .neg 							;
	jmp .end 							;

.neg:
	neg rax 							; rax = rax * (-1)
	jmp .end 							;

.end:
	mov rdx, rcx 						; rdx = len, причем, опять же, если число не считано, то rdx = 0
	ret 								

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
	call string_length 					; проверка на то, помещается ли строка в буффер
	cmp rax, rdx 						; 
	jae .long 							; если нет, то идем в .long
	xor rax, rax 						; i = 0
.loop:
	mov rcx, [rdi + rax] 				; rcx = str[i]
	mov [rsi+rax], rcx 					; buf[i] = rcx
	cmp byte [rdi + rax], 0 			; if(str[i] == 0) goto .end
	je .end 							; 
	inc rax 							; i++
	jmp .loop 							; 
.end:
	inc rax 							; делаем последний i++
	ret 		
.long:
	xor rax, rax 						; если строка не помещается, то возвращаем 0
	ret
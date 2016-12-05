all:
	gcc -framework OpenGL -framework Cocoa -lglfw3 ezview.c -o ezview


clean:
	rm -rf ezview *~

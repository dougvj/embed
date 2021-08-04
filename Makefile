embed: embed.c
	$(CC) embed.c -o embed

clean:
	-rm embed

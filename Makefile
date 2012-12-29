//CFLAGS=-g -lraphters_dynamic -L/user/lib/mysql -lmysqlclient -lhiredis
CFLAGS=-g -lfcgi -L/user/lib/mysql -lmysqlclient -lhiredis -lpthread -levent
OBJS=simple.o nginx.o sql.o cache.o json.o framework.o session.o tree.o app.o handler.o request.o response.o util.o tpl.o command.o list.o notifier.o module.o redis.o opool.o ini.o event.o connection.o async.o
run: $(OBJS)
	gcc $(OBJS) -o run $(CFLAGS)
nginx.o:nginx.c
# 	gcc -c nginx.c -o nginx.o $(CFLAGS)
simple.o:simple.c
# 	gcc -c simple.c -o simple.o -c $(CFLAGS)
sql.o:sql.c
# 	gcc -c sql.c -o sql.o -c $(CFLAGS)
cache.o:cache.c
# 	gcc -c cache.c -o cache.o -c $(CFLAGS)
json.o:json.c
# 	gcc -c json.c -o json.o -c $(CFLAGS)
session.o:session.c
# 	gcc -c session.c -o session.o -c $(CFLAGS)
framework.o:framework.c
# 	gcc -c framework.c -o framework.o -c $(CFLAGS)
clean:
	@rm *.o

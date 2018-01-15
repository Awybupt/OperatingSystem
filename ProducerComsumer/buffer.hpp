//
//  buffer.hpp
//  Producer&&Comsumer
//
//  Created by 魏晓 on 10/29/17.
//  Copyright © 2017 魏晓. All rights reserved.
//

#ifndef buffer_h
#define buffer_h
typedef int buffer_item;

#define BUFFER_SIZE 10

typedef struct{
    buffer_item data[BUFFER_SIZE];
    int head,tail;
}bufferQueue;

int insert_item(bufferQueue *,buffer_item);
int remove_item(bufferQueue *);
#endif /* buffer_h */

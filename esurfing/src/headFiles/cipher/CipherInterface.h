#ifndef CIPHER_INTERFACE_H
#define CIPHER_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cipher_interface {
    char* (*encrypt)(struct cipher_interface* self, const char* text);
    char* (*decrypt)(struct cipher_interface* self, const char* hex);
    void (*destroy)(struct cipher_interface* self);
    void* private_data;
} cipher_interface_t;

cipher_interface_t* cipherFactoryCreate(const char* algorithm_id);

void cipherFactoryDestroy(cipher_interface_t* cipher);

#ifdef __cplusplus
}
#endif

#endif // CIPHER_INTERFACE_H
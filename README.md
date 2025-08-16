# Parallel Encryption & Decryption (Demo)
- Parallel XOR-based encryption/decryption using fork + pthreads + POSIX shared memory.
- Replace XOR with AES(OpenSSL) for production.

## Build
make

## Run
./parallel_crypto input.txt output.enc
./parallel_crypto -d output.enc output.dec
diff input.txt output.dec

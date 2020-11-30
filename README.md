# Key Value Store

A generic key-value store on SGX.

<details>
<summary>Dependencies<summary>

Please ensure that you build SGX Driver, SGX PSW, and SGX SDK from source. Note that cloning from the repositories will get the current copy. You will have to create a new branch using tags for the specified versions for both the SDK and the Driver.

| Dep        |                                Details                                 |
| :--------- | :--------------------------------------------------------------------: |
| OS         |                              Ubuntu 16.04                              |
| SGX Driver | [v2.0](https://github.com/intel/linux-sgx-driver/tree/sgx_driver_2.0/) |
| SGX SDK    |        [v1.8](https://github.com/intel/linux-sgx/tree/sgx_1.8/)        |
</details>

---

- Build
    ```shell
    $ make
    ```
- Run
    ```shell
    $ ./app
    ```
- Clean
    ```shell
    $ make clean
    ```
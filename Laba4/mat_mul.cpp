#include <sycl/sycl.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>

const int M = 512;
const int N = 512;
const int P = 512;

// алгоритм на CPU
void matrixMultiplyCPU(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C) {
    for (int row = 0; row < M; ++row) {
        for (int col = 0; col < P; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < N; ++k) {
                sum += A[row * N + k] * B[k * P + col];
            }
            C[row * P + col] = sum;
        }
    }
}

int main() {
    std::vector<float> h_A(M * N);
    std::vector<float> h_B(N * P);
    std::vector<float> h_C_cpu(M * P, 0.0f);
    std::vector<float> h_C_gpu(M * P, 0.0f);

    std::default_random_engine gen(42);
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    for (int i = 0; i < M * N; ++i) h_A[i] = dis(gen);
    for (int i = 0; i < N * P; ++i) h_B[i] = dis(gen);

    // замер времени на CPU
    auto start_cpu = std::chrono::high_resolution_clock::now();
    matrixMultiplyCPU(h_A, h_B, h_C_cpu);
    auto end_cpu = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_cpu = end_cpu - start_cpu;
    std::cout << "CPU время: " << time_cpu.count() << " секунд" << std::endl;

    // вычисления на GPU
    try {
        sycl::queue q(sycl::gpu_selector_v);
        std::cout << "Запуск матричного умножения на "
                  << q.get_device().get_info<sycl::info::device::name>() << std::endl;

        sycl::buffer<float, 2> bufA(h_A.data(), sycl::range<2>(M, N));
        sycl::buffer<float, 2> bufB(h_B.data(), sycl::range<2>(N, P));
        sycl::buffer<float, 2> bufC(h_C_gpu.data(), sycl::range<2>(M, P));

        auto start_gpu = std::chrono::high_resolution_clock::now();

        q.submit([&](sycl::handler& h) {

            auto accA = bufA.get_access<sycl::access::mode::read>(h);
            auto accB = bufB.get_access<sycl::access::mode::read>(h);
            auto accC = bufC.get_access<sycl::access::mode::write>(h);

            h.parallel_for(sycl::range<2>(M, P), [=](sycl::id<2> id) {
                int row = id[0];
                int col = id[1];

                float sum = 0.0f;
                for (int k = 0; k < N; ++k) {
                    sum += accA[row][k] * accB[k][col];
                }
                accC[row][col] = sum;
            });
        }).wait();

        auto end_gpu = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_gpu = end_gpu - start_gpu;
        std::cout << "GPU время: " << time_gpu.count() << " секунд" << std::endl;

    } catch (const sycl::exception& e) {
        std::cerr << "SYCL ошибка: " << e.what() << std::endl;
        return 1;
    }

    // проверка
    bool correct = true;
    for (int i = 0; i < M * P; ++i) {
        if (std::abs(h_C_cpu[i] - h_C_gpu[i]) > 1e-4f) {
            correct = false;
            break;
        }
    }
    std::cout << "Проверка: " << (correct ? "PASSED" : "FAILED") << "\n";

    return 0;
}

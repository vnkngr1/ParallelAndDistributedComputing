#include <sycl/sycl.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>

const int WIDTH = 1024;
const int HEIGHT = 1024;
const int TS = 16; // размер локальной тайловой группы

// фильтрация на CPU
void blurFilterCPU(const std::vector<unsigned char>& input, std::vector<unsigned char>& output) {
    for (int y = 1; y < HEIGHT - 1; ++y) {
        for (int x = 1; x < WIDTH - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    sum += input[(y + ky) * WIDTH + (x + kx)];
                }
            }
            output[y * WIDTH + x] = static_cast<unsigned char>(sum / 9);
        }
    }
}

int main() {
    std::vector<unsigned char> h_input(WIDTH * HEIGHT);
    std::vector<unsigned char> h_output_cpu(WIDTH * HEIGHT, 0);
    std::vector<unsigned char> h_output_gpu(WIDTH * HEIGHT, 0);

    // заполнение случайными пикселями
    std::default_random_engine gen(42);
    std::uniform_int_distribution<int> dis(0, 255);
    for (int i = 0; i < WIDTH * HEIGHT; ++i) h_input[i] = static_cast<unsigned char>(dis(gen));

    // запуск на CPU
    auto start_cpu = std::chrono::high_resolution_clock::now();
    blurFilterCPU(h_input, h_output_cpu);
    auto end_cpu = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_cpu = end_cpu - start_cpu;
    std::cout << "время CPU фильтр размытия: " << time_cpu.count() << " секунд" << std::endl;

    // запуск на GPU с использованием локальной памяти
    try {
        sycl::queue q(sycl::gpu_selector_v);
        std::cout << "запуск фильтра на :" << q.get_device().get_info<sycl::info::device::name>() << std::endl;

        sycl::buffer<unsigned char, 1> bufIn(h_input.data(), sycl::range<1>(WIDTH * HEIGHT));
        sycl::buffer<unsigned char, 1> bufOut(h_output_gpu.data(), sycl::range<1>(WIDTH * HEIGHT));

        auto start_gpu = std::chrono::high_resolution_clock::now();

        q.submit([&](sycl::handler& h) {
            auto accIn = bufIn.get_access<sycl::access::mode::read>(h);
            auto accOut = bufOut.get_access<sycl::access::mode::write>(h);

            // локальная память под блок 16x16 плюс ореолы по 1 пикселю со всех сторон
            sycl::local_accessor<unsigned char, 2> localData(sycl::range<2>(TS + 2, TS + 2), h);

            sycl::nd_range<2> execution_range(sycl::range<2>(HEIGHT, WIDTH), sycl::range<2>(TS, TS));

            h.parallel_for(execution_range, [=](sycl::nd_item<2> item) {
                int g_y = item.get_global_id(0);
                int g_x = item.get_global_id(1);

                int l_y = item.get_local_id(0) + 1; // сдвиг
                int l_x = item.get_local_id(1) + 1;

                // загрузка центрального элемента в локальную память
                localData[l_y][l_x] = accIn[g_y * WIDTH + g_x];

                // колеткивная загрузка границ Work-Group в локальную память
                if (item.get_local_id(0) == 0) {
                    localData[0][l_x] = (g_y > 0) ? accIn[(g_y - 1) * WIDTH + g_x] : accIn[g_y * WIDTH + g_x];
                }
                if (item.get_local_id(0) == TS - 1) {
                    localData[TS + 1][l_x] = (g_y < HEIGHT - 1) ? accIn[(g_y + 1) * WIDTH + g_x] : accIn[g_y * WIDTH + g_x];
                }
                if (item.get_local_id(1) == 0) {
                    localData[l_y][0] = (g_x > 0) ? accIn[g_y * WIDTH + (g_x - 1)] : accIn[g_y * WIDTH + g_x];
                }
                if (item.get_local_id(1) == TS - 1) {
                    localData[l_y][TS + 1] = (g_x < WIDTH - 1) ? accIn[g_y * WIDTH + (g_x + 1)] : accIn[g_y * WIDTH + g_x];
                }

                // угловые элементы для корректного размытия по диагоналям
                if (item.get_local_id(0) == 0 && item.get_local_id(1) == 0) {
                    localData[0][0] = (g_y > 0 && g_x > 0) ? accIn[(g_y - 1) * WIDTH + (g_x - 1)] : accIn[g_y * WIDTH + g_x];
                }
                if (item.get_local_id(0) == 0 && item.get_local_id(1) == TS - 1) {
                    localData[0][TS + 1] = (g_y > 0 && g_x < WIDTH - 1) ? accIn[(g_y - 1) * WIDTH + (g_x + 1)] : accIn[g_y * WIDTH + g_x];
                }
                if (item.get_local_id(0) == TS - 1 && item.get_local_id(1) == 0) {
                    localData[TS + 1][0] = (g_y < HEIGHT - 1 && g_x > 0) ? accIn[(g_y + 1) * WIDTH + (g_x - 1)] : accIn[g_y * WIDTH + g_x];
                }
                if (item.get_local_id(0) == TS - 1 && item.get_local_id(1) == TS - 1) {
                    localData[TS + 1][TS + 1] = (g_y < HEIGHT - 1 && g_x < WIDTH - 1) ? accIn[(g_y + 1) * WIDTH + (g_x + 1)] : accIn[g_y * WIDTH + g_x];
                }

                item.barrier(sycl::access::fence_space::local_space);

                // только для внутренних пикселей картинки, границы пропуск
                if (g_y > 0 && g_y < HEIGHT - 1 && g_x > 0 && g_x < WIDTH - 1) {
                    int sum = 0;
                    for (int ky = -1; ky <= 1; ++ky) {
                        for (int kx = -1; kx <= 1; ++kx) {
                            sum += localData[l_y + ky][l_x + kx];
                        }
                    }
                    accOut[g_y * WIDTH + g_x] = static_cast<unsigned char>(sum / 9);
                } else {
                    accOut[g_y * WIDTH + g_x] = accIn[g_y * WIDTH + g_x]; // края не трогать
                }
            });
        }).wait();

        auto end_gpu = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_gpu = end_gpu - start_gpu;
        std::cout << "время GPU фильтр размытия: " << time_gpu.count() << " секунд" << std::endl;

    } catch (const sycl::exception& e) {
        std::cerr << "SYCL ошибка: " << e.what() << std::endl;
        return 1;
    }

    // сравнение результатов
    bool correct = true;
    for (int y = 1; y < HEIGHT - 1; ++y) {
        for (int x = 1; x < WIDTH - 1; ++x) {
            if (h_output_cpu[y * WIDTH + x] != h_output_gpu[y * WIDTH + x]) {
                correct = false;
                break;
            }
        }
    }
    std::cout << "проверка: " << (correct ? "хорошо" : "плохо") << std::endl;

    return 0;
}
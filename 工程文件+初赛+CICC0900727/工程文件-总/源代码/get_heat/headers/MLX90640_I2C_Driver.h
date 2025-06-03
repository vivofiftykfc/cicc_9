/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef MLX90640_I2C_DRIVER_H
#define MLX90640_I2C_DRIVER_H

#include <stdint.h>
#include <cstdint>

    int MLX90640_I2CInit(void);
    int MLX90640_I2CRead(uint8_t addr, uint16_t reg, uint16_t cnt, uint16_t *data);
    int MLX90640_I2CWrite(uint8_t addr, uint16_t reg, uint16_t val);
#endif
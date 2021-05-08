/* Copyright 2018 Canaan Inc.
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
 */

#include "types.h"
#include "riscv.h"
#include "utils.h"
#include "dmac.h"
#include "spi.h"
#include "sysctl.h"
#include "memlayout.h"
#include "string.h"

volatile spi_t *const spi[4] =
    {
        (volatile spi_t *)SPI0_BASE_ADDR,
        (volatile spi_t *)SPI1_BASE_ADDR,
        (volatile spi_t *)SPI_SLAVE_BASE_ADDR,
        (volatile spi_t *)SPI3_BASE_ADDR};

static spi_frame_format_t spi_get_frame_format(spi_device_num_t spi_num)
{
    uint8 frf_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            frf_offset = 21;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            frf_offset = 22;
            break;
    }
    volatile spi_t *spi_adapter = spi[spi_num];
    return ((spi_adapter->ctrlr0 >> frf_offset) & 0x3);
}

static spi_transfer_width_t spi_get_frame_size(size_t data_bit_length)
{
    if(data_bit_length < 8)
        return SPI_TRANS_CHAR;
    else if(data_bit_length < 16)
        return SPI_TRANS_SHORT;
    return SPI_TRANS_INT;
}

static void spi_set_tmod(uint8 spi_num, uint32 tmod)
{
    configASSERT(spi_num < SPI_DEVICE_MAX);
    volatile spi_t *spi_handle = spi[spi_num];
    uint8 tmod_offset = 0;
    switch(spi_num)
    {
        case 0:
        case 1:
        case 2:
            tmod_offset = 8;
            break;
        case 3:
        default:
            tmod_offset = 10;
            break;
    }
    set_bit(&spi_handle->ctrlr0, 3 << tmod_offset, tmod << tmod_offset);
}

void spi_init(spi_device_num_t spi_num, spi_work_mode_t work_mode, spi_frame_format_t frame_format,
              size_t data_bit_length, uint32 endian)
{
    configASSERT(data_bit_length >= 4 && data_bit_length <= 32);
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);

    uint8 dfs_offset, frf_offset, work_mode_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            frf_offset = 21;
            work_mode_offset = 6;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            frf_offset = 22;
            work_mode_offset = 8;
            break;
    }

    switch(frame_format)
    {
        case SPI_FF_DUAL:
            configASSERT(data_bit_length % 2 == 0);
            break;
        case SPI_FF_QUAD:
            configASSERT(data_bit_length % 4 == 0);
            break;
        case SPI_FF_OCTAL:
            configASSERT(data_bit_length % 8 == 0);
            break;
        default:
            break;
    }
    volatile spi_t *spi_adapter = spi[spi_num];
    if(spi_adapter->baudr == 0)
        spi_adapter->baudr = 0x14;
    spi_adapter->imr = 0x00;
    spi_adapter->dmacr = 0x00;
    spi_adapter->dmatdlr = 0x10;
    spi_adapter->dmardlr = 0x00;
    spi_adapter->ser = 0x00;
    spi_adapter->ssienr = 0x00;
    spi_adapter->ctrlr0 = (work_mode << work_mode_offset) | (frame_format << frf_offset) | ((data_bit_length - 1) << dfs_offset);
    spi_adapter->spi_ctrlr0 = 0;
    spi_adapter->endian = endian;
}

void spi_send_data_normal(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *tx_buff, size_t tx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);

    size_t index, fifo_len;
    spi_set_tmod(spi_num, SPI_TMOD_TRANS);

    volatile spi_t *spi_handle = spi[spi_num];

    uint8 dfs_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            break;
    }
    uint32 data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = spi_get_frame_size(data_bit_length);

    uint8 v_misalign_flag = 0;
    uint32 v_send_data;
    if((uintptr_t)tx_buff % frame_width)
        v_misalign_flag = 1;

    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32 i = 0;
    while(tx_len)
    {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                fifo_len = fifo_len / 4 * 4;
                if(v_misalign_flag)
                {
                    for(index = 0; index < fifo_len; index += 4)
                    {
                        memcpy(&v_send_data, tx_buff + i, 4);
                        spi_handle->dr[0] = v_send_data;
                        i += 4;
                    }
                } else
                {
                    for(index = 0; index < fifo_len / 4; index++)
                        spi_handle->dr[0] = ((uint32 *)tx_buff)[i++];
                }
                break;
            case SPI_TRANS_SHORT:
                fifo_len = fifo_len / 2 * 2;
                if(v_misalign_flag)
                {
                    for(index = 0; index < fifo_len; index += 2)
                    {
                        memcpy(&v_send_data, tx_buff + i, 2);
                        spi_handle->dr[0] = v_send_data;
                        i += 2;
                    }
                } else
                {
                    for(index = 0; index < fifo_len / 2; index++)
                        spi_handle->dr[0] = ((uint16 *)tx_buff)[i++];
                }
                break;
            default:
                for(index = 0; index < fifo_len; index++)
                    spi_handle->dr[0] = tx_buff[i++];
                break;
        }
        tx_len -= fifo_len;
    }
    while((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

void spi_send_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *cmd_buff,
                            size_t cmd_len, const uint8 *tx_buff, size_t tx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    uint8 *v_buf = malloc(cmd_len + tx_len);
    size_t i;
    for(i = 0; i < cmd_len; i++)
        v_buf[i] = cmd_buff[i];
    for(i = 0; i < tx_len; i++)
        v_buf[cmd_len + i] = tx_buff[i];

    spi_send_data_normal(spi_num, chip_select, v_buf, cmd_len + tx_len);
    free((void *)v_buf);
}

void spi_send_data_standard_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                                spi_chip_select_t chip_select,
                                const uint8 *cmd_buff, size_t cmd_len, const uint8 *tx_buff, size_t tx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);

    volatile spi_t *spi_handle = spi[spi_num];

    uint8 dfs_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            break;
    }
    uint32 data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = spi_get_frame_size(data_bit_length);

    uint32 *buf;
    size_t v_send_len;
    int i;
    switch(frame_width)
    {
        case SPI_TRANS_INT:
            buf = malloc(cmd_len + tx_len);
            for(i = 0; i < cmd_len / 4; i++)
                buf[i] = ((uint32 *)cmd_buff)[i];
            for(i = 0; i < tx_len / 4; i++)
                buf[cmd_len / 4 + i] = ((uint32 *)tx_buff)[i];
            v_send_len = (cmd_len + tx_len) / 4;
            break;
        case SPI_TRANS_SHORT:
            buf = malloc((cmd_len + tx_len) / 2 * sizeof(uint32));
            for(i = 0; i < cmd_len / 2; i++)
                buf[i] = ((uint16 *)cmd_buff)[i];
            for(i = 0; i < tx_len / 2; i++)
                buf[cmd_len / 2 + i] = ((uint16 *)tx_buff)[i];
            v_send_len = (cmd_len + tx_len) / 2;
            break;
        default:
            buf = malloc((cmd_len + tx_len) * sizeof(uint32));
            for(i = 0; i < cmd_len; i++)
                buf[i] = cmd_buff[i];
            for(i = 0; i < tx_len; i++)
                buf[cmd_len + i] = tx_buff[i];
            v_send_len = cmd_len + tx_len;
            break;
    }

    spi_send_data_normal_dma(channel_num, spi_num, chip_select, buf, v_send_len, SPI_TRANS_INT);

    free((void *)buf);
}

void spi_send_data_normal_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num,
                              spi_chip_select_t chip_select,
                              const void *tx_buff, size_t tx_len, spi_transfer_width_t spi_transfer_width)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    spi_set_tmod(spi_num, SPI_TMOD_TRANS);
    volatile spi_t *spi_handle = spi[spi_num];
    uint32 *buf;
    int i;
    switch(spi_transfer_width)
    {
        case SPI_TRANS_SHORT:
#if FIX_CACHE
            buf = (uint32 *)iomem_malloc((tx_len) * sizeof(uint32));
#else
            buf = (uint32 *)malloc((tx_len) * sizeof(uint32));
#endif
            for(i = 0; i < tx_len; i++)
                buf[i] = ((uint16 *)tx_buff)[i];
            break;
        case SPI_TRANS_INT:
            buf = (uint32 *)tx_buff;
            break;
        case SPI_TRANS_CHAR:
        default:
#if FIX_CACHE
            buf = (uint32 *)iomem_malloc((tx_len) * sizeof(uint32));
#else
            buf = (uint32 *)malloc((tx_len) * sizeof(uint32));
#endif

            for(i = 0; i < tx_len; i++)
                buf[i] = ((uint8 *)tx_buff)[i];
            break;
    }
    spi_handle->dmacr = 0x2; /*enable dma transmit*/
    spi_handle->ssienr = 0x01;

    sysctl_dma_select((sysctl_dma_channel_t)channel_num, SYSCTL_DMA_SELECT_SSI0_TX_REQ + spi_num * 2);

    dmac_set_single_mode(channel_num, buf, (void *)(&spi_handle->dr[0]), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
                         DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, tx_len);
    spi_handle->ser = 1U << chip_select;
    dmac_wait_done(channel_num);
    if(spi_transfer_width != SPI_TRANS_INT)
    {
#if FIX_CACHE
        iomem_free((void *)buf);
#else
        free((void *)buf);
#endif
    }
    while((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

void spi_receive_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *cmd_buff,
                               size_t cmd_len, uint8 *rx_buff, size_t rx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    size_t index, fifo_len;
    if(cmd_len == 0)
        spi_set_tmod(spi_num, SPI_TMOD_RECV);
    else
        spi_set_tmod(spi_num, SPI_TMOD_EEROM);
    volatile spi_t *spi_handle = spi[spi_num];

    uint8 dfs_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            break;
    }
    uint32 data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = spi_get_frame_size(data_bit_length);

    uint32 i = 0;
    size_t v_cmd_len = cmd_len / frame_width;
    uint32 v_rx_len = rx_len / frame_width;

    spi_handle->ctrlr1 = (uint32)(v_rx_len - 1);
    spi_handle->ssienr = 0x01;

    while(v_cmd_len)
    {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < v_cmd_len ? fifo_len : v_cmd_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                for(index = 0; index < fifo_len; index++)
                    spi_handle->dr[0] = ((uint32 *)cmd_buff)[i++];
                break;
            case SPI_TRANS_SHORT:
                for(index = 0; index < fifo_len; index++)
                    spi_handle->dr[0] = ((uint16 *)cmd_buff)[i++];
                break;
            default:
                for(index = 0; index < fifo_len; index++)
                    spi_handle->dr[0] = cmd_buff[i++];
                break;
        }
        spi_handle->ser = 1U << chip_select;
        v_cmd_len -= fifo_len;
    }

    if(cmd_len == 0)
    {
        spi_handle->dr[0] = 0xffffffff;
        spi_handle->ser = 1U << chip_select;
    }

    i = 0;
    while(v_rx_len)
    {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < v_rx_len ? fifo_len : v_rx_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                for(index = 0; index < fifo_len; index++)
                    ((uint32 *)rx_buff)[i++] = spi_handle->dr[0];
                break;
            case SPI_TRANS_SHORT:
                for(index = 0; index < fifo_len; index++)
                    ((uint16 *)rx_buff)[i++] = (uint16)spi_handle->dr[0];
                break;
            default:
                for(index = 0; index < fifo_len; index++)
                    rx_buff[i++] = (uint8)spi_handle->dr[0];
                break;
        }

        v_rx_len -= fifo_len;
    }

    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

void spi_receive_data_normal_dma(dmac_channel_number_t dma_send_channel_num,
                                 dmac_channel_number_t dma_receive_channel_num,
                                 spi_device_num_t spi_num, spi_chip_select_t chip_select, const void *cmd_buff,
                                 size_t cmd_len, void *rx_buff, size_t rx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);

    if(cmd_len == 0)
        spi_set_tmod(spi_num, SPI_TMOD_RECV);
    else
        spi_set_tmod(spi_num, SPI_TMOD_EEROM);

    volatile spi_t *spi_handle = spi[spi_num];

    spi_handle->ctrlr1 = (uint32)(rx_len - 1);
    spi_handle->dmacr = 0x3;
    spi_handle->ssienr = 0x01;
    if(cmd_len)
        sysctl_dma_select((sysctl_dma_channel_t)dma_send_channel_num, SYSCTL_DMA_SELECT_SSI0_TX_REQ + spi_num * 2);
    sysctl_dma_select((sysctl_dma_channel_t)dma_receive_channel_num, SYSCTL_DMA_SELECT_SSI0_RX_REQ + spi_num * 2);

    dmac_set_single_mode(dma_receive_channel_num, (void *)(&spi_handle->dr[0]), rx_buff, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, rx_len);
    if(cmd_len)
        dmac_set_single_mode(dma_send_channel_num, cmd_buff, (void *)(&spi_handle->dr[0]), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
                             DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, cmd_len);
    if(cmd_len == 0 && spi_get_frame_format(spi_num) == SPI_FF_STANDARD)
        spi[spi_num]->dr[0] = 0xffffffff;
    spi_handle->ser = 1U << chip_select;
    if(cmd_len)
        dmac_wait_done(dma_send_channel_num);
    dmac_wait_done(dma_receive_channel_num);

    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

void spi_receive_data_standard_dma(dmac_channel_number_t dma_send_channel_num,
                                   dmac_channel_number_t dma_receive_channel_num,
                                   spi_device_num_t spi_num, spi_chip_select_t chip_select, const uint8 *cmd_buff,
                                   size_t cmd_len, uint8 *rx_buff, size_t rx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    volatile spi_t *spi_handle = spi[spi_num];

    uint8 dfs_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            break;
    }
    uint32 data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = spi_get_frame_size(data_bit_length);

    size_t i;

    uint32 *write_cmd;
    uint32 *read_buf;
    size_t v_recv_len;
    size_t v_cmd_len;
    switch(frame_width)
    {
        case SPI_TRANS_INT:
#if FIX_CACHE
            write_cmd = iomem_malloc(cmd_len + rx_len);
#else
            write_cmd = malloc(cmd_len + rx_len);
#endif
            for(i = 0; i < cmd_len / 4; i++)
                write_cmd[i] = ((uint32 *)cmd_buff)[i];
            read_buf = &write_cmd[i];
            v_recv_len = rx_len / 4;
            v_cmd_len = cmd_len / 4;
            break;
        case SPI_TRANS_SHORT:
#if FIX_CACHE
            write_cmd = iomem_malloc((cmd_len + rx_len) / 2 * sizeof(uint32));
#else
            write_cmd = malloc((cmd_len + rx_len) / 2 * sizeof(uint32));
#endif
            for(i = 0; i < cmd_len / 2; i++)
                write_cmd[i] = ((uint16 *)cmd_buff)[i];
            read_buf = &write_cmd[i];
            v_recv_len = rx_len / 2;
            v_cmd_len = cmd_len / 2;
            break;
        default:
#if FIX_CACHE
            write_cmd = iomem_malloc((cmd_len + rx_len) * sizeof(uint32));
#else
            write_cmd = malloc((cmd_len + rx_len) * sizeof(uint32));
#endif
            for(i = 0; i < cmd_len; i++)
                write_cmd[i] = cmd_buff[i];
            read_buf = &write_cmd[i];
            v_recv_len = rx_len;
            v_cmd_len = cmd_len;
            break;
    }

    spi_receive_data_normal_dma(dma_send_channel_num, dma_receive_channel_num, spi_num, chip_select, write_cmd, v_cmd_len, read_buf, v_recv_len);

    switch(frame_width)
    {
        case SPI_TRANS_INT:
            for(i = 0; i < v_recv_len; i++)
                ((uint32 *)rx_buff)[i] = read_buf[i];
            break;
        case SPI_TRANS_SHORT:
            for(i = 0; i < v_recv_len; i++)
                ((uint16 *)rx_buff)[i] = read_buf[i];
            break;
        default:
            for(i = 0; i < v_recv_len; i++)
                rx_buff[i] = read_buf[i];
            break;
    }
#if FIX_CACHE
    iomem_free(write_cmd);
#else
    free(write_cmd);
#endif
}

void spi_fill_data_dma(dmac_channel_number_t channel_num, spi_device_num_t spi_num, spi_chip_select_t chip_select,
                       const uint32 *tx_buff, size_t tx_len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);

    spi_set_tmod(spi_num, SPI_TMOD_TRANS);
    volatile spi_t *spi_handle = spi[spi_num];
    spi_handle->dmacr = 0x2; /*enable dma transmit*/
    spi_handle->ssienr = 0x01;

    sysctl_dma_select((sysctl_dma_channel_t)channel_num, SYSCTL_DMA_SELECT_SSI0_TX_REQ + spi_num * 2);
    dmac_set_single_mode(channel_num, tx_buff, (void *)(&spi_handle->dr[0]), DMAC_ADDR_NOCHANGE, DMAC_ADDR_NOCHANGE,
                         DMAC_MSIZE_1, DMAC_TRANS_WIDTH_32, tx_len);
    spi_handle->ser = 1U << chip_select;
    dmac_wait_done(channel_num);

    while((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}
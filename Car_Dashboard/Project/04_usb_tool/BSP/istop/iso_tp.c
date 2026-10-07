#include "iso_tp.h"
#include "canif.h"

/* ISO15765-2 ֡���Ͷ��壬�������ֽڸ�4bit����֡���� */
#define ISO_TP_FF  0x10    /* ��֡ First Frame   �����ݷְ��ĵ�һ�� */
#define ISO_TP_CF  0x20    /* ����֡ Consecutive Frame FF֮��ķְ� */
#define ISO_TP_FC  0x30    /* ����֡ Flow Control ���ն˷������Ͷ˵�����Ӧ�� */

#define OTA_FRAME_HEADER  0xBF  /* �Զ���OTA���ݰ�ħ��ͷ��������ʶ��һҳ�̼��� */

/**
 * @brief ISO-TP����ģ���ʼ��
 * @note ���Ͷ����ﲻ��Ҫά������״̬��������Ϊ�պ��������ն�Bootloader����Ҫ����״̬����
 */
void iso_tp_init(void)
{
    /* ���Ͷ����������ʼ�� */
}

/**
 * @brief �ȴ����ն�(Bootloader)�ظ�FC����֡ ���� һҳ������ɵ�ACKӦ��
 * @param  timeout: ��ʱʱ�䣬��λms
 * @retval 0:�յ�FC/ҳACK��1:�ȴ���ʱ
 * @note  ѭ����ȡCAN���ģ����CAN_ID_OTA_RECV����
 *        ���ֺϷ�Ӧ��ISO-TP��׼FC����֡ / �Զ���ҳACK(���ֽ�0xBF)
 */
static uint8_t wait_flow_control(uint32_t timeout)
{
    uint32_t start = HAL_GetTick();  //��¼�ȴ���ʼʱ���
    // �ڳ�ʱʱ����ѭ����ȡCAN����
    while (HAL_GetTick() - start < timeout)
    {
        uint32_t id;
        uint8_t  data[8];
        // ��ȡһ֡CAN���ģ�����ֵ>0�����յ�����
        if (Can_Recv_Msg(&id, data) > 0)
        {
            // �жϱ���ID�Ƿ�ΪBootloader�ظ���ID
            if (id == CAN_ID_OTA_RECV)
            {
                // �жϣ��Ǳ�׼ISO-TP����FC֡���������Զ���ҳACK֡
                if ((data[0] & 0xF0) == ISO_TP_FC || data[0] == 0xBF)
                {
                    return 0;  //�յ��Ϸ�Ӧ��ֱ�ӷ��سɹ�
                }
            }
        }
    }
    return 1;  //�ȴ���ʱ��û���յ�Ӧ�𣬷���ʧ��
}

/**
 * @brief ͨ�� ISO-TP ����һҳ�̼����ݣ����ͷ����ĺ�����
 * @param offset �̼���Flash��д��ƫ�Ƶ�ַ(4�ֽ�)
 * @param data ԭʼ�̼����ݻ�����ָ��
 * @param len ԭʼ�̼���Ч���ݳ���
 * @retval 0 �ɹ�, 1 ʧ�ܣ��ȴ�FC��ʱ��
 * @note ��װ����OTA���ṹ����0xBF(1) + offset(4) + len(4) + �̼�data(len)��
 *       �ܳ��� total_len = len + 9
 *       ��total_len <=7��ʹ��SF��֡���ͣ�����7ʹ��FF+CF��֡ISO-TP�ְ�
 */
uint8_t iso_tp_send_data(uint32_t offset, uint8_t *data, uint16_t len)
{
    uint8_t  frame[8];      //CAN���Ļ��棬�������8�ֽ�
    uint16_t sent = 0;      //�Ѿ����͵�ISO-TP�غ��ֽڼ���
    uint8_t  seq = 1;       //CF����֡��ţ�FF֮���һ��CF��ű����1��ʼ

    /*
     * ��װ�ϲ�OTAӦ�ð��ܳ���
     * OTAӦ�ð� = 1�ֽ�ħ��(0xBF) + 4�ֽ�Flashƫ��offset +4�ֽڹ̼�����len + len�ֽڹ̼�����
     * 1+4+4 =9�ֽ�ͷ�������Ϲ̼�ԭʼ����len���õ�����ISO-TPҪ��������غɳ���
     */
    uint16_t total_len = len + 9;

    /* SF branch removed: OTA header is 9 bytes, total_len is always >= 13 (> 7),
     * so every page uses FF + CF. The old SF code was dead AND packed offset/len
     * with native little-endian 32-bit writes, breaking the big-endian protocol. */

    // ========== ��֧2�����غ�>7�ֽڣ���֡���䣺����FF��֡ ==========
    /* FF��֡Byte0����4bit=0x10(FF)����4bit���ܳ��ȵĸ�4bit��Byte1Ϊ�ܳ��ȵ�8bit */
    frame[0] = ISO_TP_FF | (total_len >> 8);
    frame[1] = total_len & 0xFF;

    //���FF��������Я����6�ֽ�Ӧ���غ�
    frame[2] = OTA_FRAME_HEADER;                     //Ӧ�ð���0�ֽڣ�ħ��0xBF
    frame[3] = (offset >> 24) & 0xFF;                //offset ����ֽ�
    frame[4] = (offset >> 16) & 0xFF;
    frame[5] = (offset >> 8) & 0xFF;
    frame[6] = offset & 0xFF;                        //offset����ֽ�
    frame[7] = (len >> 24) & 0xFF;                   //len����ֽ�

    //����FF��֡��DLC�̶�8�ֽ�
    Can_Send_Msg(CAN_ID_OTA_SEND, 8, frame);
    sent = 6;  //FF��֡����Я����6�ֽ�Ӧ�ò��غɣ�����ѷ����ֽ���

    /* ������FF�������ȴ�Bootloader�ظ�FC����֡��1000ms��ʱ */
    if (wait_flow_control(1000) != 0)
        return 1;

    // ========== ����CF����֡ѭ�� ==========
    //ѭ������CF��ֱ��sent�ۼƵ���total_len������Ӧ���غɷ������
    while (sent < total_len)
    {
        frame[0] = ISO_TP_CF | (seq & 0x0F); //CF֡����4bit=0x20(CF)����4bit=seq���
        uint8_t data_len = 7;               //CFÿ�����Я��7�ֽ�Ӧ���غɣ��۳�1�ֽ�֡ͷ��
        //ʣ�಻��7�ֽڣ�ȡʣ���ֽ�������ֹԽ��
        if (sent + data_len > total_len)
        {
            data_len = total_len - sent;
        }

        //��䵱ǰCF���ڵ�Ӧ���غ�
        for (uint8_t i = 0; i < data_len; i++)
        {
            if (sent + i < 9)
            {
                /* ��ǰλ������OTAͷ������0~8�ţ���9�ֽڣ�0xBF + offset4 + len4�� */
                if (sent + i == 0)
                    frame[1 + i] = OTA_FRAME_HEADER;
                else if (sent + i >= 1 && sent + i <= 4)
                    //offset4�ֽڣ�����1~4
                    frame[1 + i] = (offset >> (8 * (4 - (sent + i)))) & 0xFF;
                else if (sent + i >= 5 && sent + i <= 8)
                    //len4�ֽڣ�����5~8
                    frame[1 + i] = (len >> (8 * (8 - (sent + i)))) & 0xFF;
            }
            else
            {
                /* ����ǰ9�ֽ�ͷ����������ԭʼ�̼����ݣ�ֱ�ӿ��� */
                frame[1 + i] = data[sent + i - 9];
            }
        }
        //���͵�ǰCF���ģ�DLC = ֡ͷ1�ֽ� + ��Ч�غ�data_len
        Can_Send_Msg(CAN_ID_OTA_SEND, data_len + 1, frame);
        sent += data_len;  //�ۼ��ѷ����ֽ�
        seq++;             //CF�������

        /* BS=0 mode: receiver sends exactly one FC after FF, never again.
         * The old 'wait FC every 8 CFs' logic dead-locked against BS=0. */
    }

    /* ��ǰ��һҳ����CF������ϣ��ȴ�Bootloader����ҳACK��2000ms��ʱ */
    wait_flow_control(2000);
    return 0;
}

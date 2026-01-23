#!/usr/bin/env python3
"""
ESP32-S3 LCD测试脚本
通过串口发送测试命令并验证LCD显示功能
"""

import serial
import time
import sys
import argparse

def send_command(ser, cmd, desc, test_no):
    """发送命令到ESP32-S3"""
    print(f"\n[测试 {test_no}] {desc}")
    print(f"发送命令: {cmd}")
    ser.write(f"{cmd}\n".encode())
    ser.flush()
    time.sleep(3)  # 等待显示
    print("✓ 完成")

def run_tests(port, baudrate):
    """运行所有测试用例"""
    print("=== ESP32-S3 LCD 测试脚本 ===")
    print(f"串口: {port}")
    print(f"波特率: {baudrate}")
    print()
    
    try:
        # 打开串口
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1
        )
        
        print("串口打开成功!")
        time.sleep(2)
        
        # 测试用例列表
        test_cases = [
            (1, "hello", "显示 Hello World!"),
            (2, "wave", "显示挥手图标"),
            (3, "smile", "显示笑脸图标"),
            (4, "sad", "显示哭脸图标"),
            (5, "clear", "清空屏幕"),
            (6, "hello", "连续测试 - hello"),
            (6, "smile", "连续测试 - smile"),
            (6, "wave", "连续测试 - wave"),
            (7, "unknown", "测试无效命令")
        ]
        
        print("\n开始执行测试...")
        print("=" * 60)
        
        for test_no, cmd, desc in test_cases:
            send_command(ser, cmd, desc, test_no)
        
        print("\n" + "=" * 60)
        print("所有测试完成!")
        print()
        
        # 交互模式
        print("进入交互模式（输入 'exit' 退出）：")
        while True:
            try:
                user_input = input("命令> ")
                if user_input.lower() == 'exit':
                    break
                if user_input:
                    ser.write(f"{user_input}\n".encode())
                    ser.flush()
                    print(f"已发送: {user_input}")
            except KeyboardInterrupt:
                print("\n用户中断")
                break
        
        ser.close()
        print("串口已关闭")
        
    except serial.SerialException as e:
        print(f"错误: {e}")
        print("\n请检查:")
        print("1. 串口号是否正确")
        print("2. ESP32-S3 是否已连接")
        print("3. 串口是否被其他程序占用")
        print("\n使用方法: python test_lcd.py -p COM端口号")
        print("示例: python test_lcd.py -p COM3")
        sys.exit(1)
    except Exception as e:
        print(f"未知错误: {e}")
        sys.exit(1)

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='ESP32-S3 LCD测试工具')
    parser.add_argument('-p', '--port', default='COM3', help='串口号 (默认: COM3)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='波特率 (默认: 115200)')
    
    args = parser.parse_args()
    
    run_tests(args.port, args.baudrate)

if __name__ == "__main__":
    main()

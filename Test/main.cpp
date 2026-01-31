///
/// @file main.cpp
/// @brief CTP交易API登录测试程序
///

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <csignal>
#include <atomic>
#include <memory>
#include <unistd.h>
#include <fstream>
#include <sstream>

// CTP交易API头文件
#include "ThostFtdcTraderApi.h"

// 全局变量用于控制程序退出
std::atomic<bool> g_running(true);

// 信号处理函数
void SignalHandler(int signal) {
    std::cout << "\n收到信号 " << signal << ", 准备退出程序..." << std::endl;
    g_running = false;
}

///
/// @brief CTP交易回调类
///
class TraderSpi : public CThostFtdcTraderSpi {
public:
    TraderSpi(CThostFtdcTraderApi* api) : m_api(api), m_requestId(0) {}

    /// 当客户端与交易后台建立起通信连接时，服务器主动发送登录请求
    virtual void OnFrontConnected() override {
        std::cout << "[连接] 成功连接到交易服务器" << std::endl;
        std::cout << "[状态] 开始用户登录..." << std::endl;
        ReqUserLogin();
    }

    /// 当客户端与交易后台通信连接断开时，该方法被调用
    virtual void OnFrontDisconnected(int nReason) override {
        std::cout << "[断开] 与交易服务器断开连接, 原因码: " << nReason << std::endl;
        switch (nReason) {
            case 0x1001:
                std::cout << "  原因: 网络读失败" << std::endl;
                break;
            case 0x1002:
                std::cout << "  原因: 网络写失败" << std::endl;
                break;
            case 0x2001:
                std::cout << "  原因: 接收心跳超时" << std::endl;
                break;
            case 0x2002:
                std::cout << "  原因: 发送心跳失败" << std::endl;
                break;
            case 0x2003:
                std::cout << "  原因: 收到错误报文" << std::endl;
                break;
            default:
                std::cout << "  原因: 未知" << std::endl;
                break;
        }
        g_running = false;
    }

    /// 心跳超时警告
    virtual void OnHeartBeatWarning(int nTimeLapse) override {
        std::cout << "[警告] 心跳超时, 距离上次接收时间: " << nTimeLapse << "秒" << std::endl;
    }

    /// 客户端认证响应
    virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) override {
        std::cout << "[认证] 收到认证响应, RequestID: " << nRequestID << std::endl;
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            std::cout << "[错误] 认证失败, ErrorID: " << pRspInfo->ErrorID
                      << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
        } else {
            std::cout << "[成功] 客户端认证成功" << std::endl;
        }
    }

    /// 登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) override {
        std::cout << "[登录] 收到登录响应, RequestID: " << nRequestID << std::endl;

        if (pRspInfo && pRspInfo->ErrorID != 0) {
            std::cout << "[错误] 登录失败!" << std::endl;
            std::cout << "  ErrorID: " << pRspInfo->ErrorID << std::endl;
            std::cout << "  ErrorMsg: " << (pRspInfo->ErrorMsg[0] ? pRspInfo->ErrorMsg : "无") << std::endl;
            g_running = false;
            return;
        }

        std::cout << "[成功] 登录成功!" << std::endl;

        if (pRspUserLogin) {
            std::cout << "====================================" << std::endl;
            std::cout << "登录信息:" << std::endl;
            std::cout << "  交易日:    " << pRspUserLogin->TradingDay << std::endl;
            std::cout << "  登录时间:  " << pRspUserLogin->LoginTime << std::endl;
            std::cout << "  经纪公司:  " << pRspUserLogin->BrokerID << std::endl;
            std::cout << "  用户ID:    " << pRspUserLogin->UserID << std::endl;
            std::cout << "  交易系统:  " << pRspUserLogin->SystemName << std::endl;
            std::cout << "  前端ID:    " << pRspUserLogin->FrontID << std::endl;
            std::cout << "  会话ID:    " << pRspUserLogin->SessionID << std::endl;
            std::cout << "  最大订单:  " << pRspUserLogin->MaxOrderRef << std::endl;
            std::cout << "  SHFE时间:  " << pRspUserLogin->SHFETime << std::endl;
            std::cout << "  DCHE时间:  " << pRspUserLogin->DCETime << std::endl;
            std::cout << "  CZCE时间:  " << pRspUserLogin->CZCETime << std::endl;
            std::cout << "  DCE时间:   " << pRspUserLogin->DCETime << std::endl;
            std::cout << "  INE时间:   " << pRspUserLogin->INETime << std::endl;
            std::cout << "====================================" << std::endl;
        }

        // 登录成功后查询结算信息确认
        std::cout << "[状态] 查询投资者结算信息..." << std::endl;
        ReqQrySettlementInfo();
    }

    /// 登出请求响应
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                                 CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) override {
        std::cout << "[登出] 收到登出响应, RequestID: " << nRequestID << std::endl;
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            std::cout << "[错误] 登出失败, ErrorID: " << pRspInfo->ErrorID
                      << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
        } else {
            std::cout << "[成功] 登出成功" << std::endl;
        }
        g_running = false;
    }

    /// 查询结算信息响应
    virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo,
                                        CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) override {
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            std::cout << "[错误] 查询结算信息失败, ErrorID: " << pRspInfo->ErrorID
                      << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
            // 即使查询失败也继续确认
        } else if (bIsLast) {
            std::cout << "[成功] 查询结算信息完成" << std::endl;
        }

        if (bIsLast) {
            // 查询完成后进行结算确认
            std::cout << "[状态] 确认投资者结算信息..." << std::endl;
            ReqSettlementInfoConfirm();
        }
    }

    /// 投资者结算结果确认响应
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                            CThostFtdcRspInfoField *pRspInfo,
                                            int nRequestID, bool bIsLast) override {
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            std::cout << "[错误] 结算确认失败, ErrorID: " << pRspInfo->ErrorID
                      << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
        } else {
            std::cout << "[成功] 结算信息确认成功" << std::endl;
            if (pSettlementInfoConfirm) {
                std::cout << "  确认日期: " << pSettlementInfoConfirm->ConfirmDate << std::endl;
                std::cout << "  确认时间: " << pSettlementInfoConfirm->ConfirmTime << std::endl;
            }
        }

        // 结算确认后查询资金账户
        if (bIsLast) {
            std::cout << "[状态] 查询资金账户..." << std::endl;
            ReqQryTradingAccount();
        }
    }

    /// 查询资金账户响应
    virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                        CThostFtdcRspInfoField *pRspInfo,
                                        int nRequestID, bool bIsLast) override {
        if (pRspInfo && pRspInfo->ErrorID != 0) {
            std::cout << "[错误] 查询资金账户失败, ErrorID: " << pRspInfo->ErrorID
                      << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
        } else if (pTradingAccount) {
            std::cout << "[成功] 查询资金账户成功" << std::endl;
            std::cout << "====================================" << std::endl;
            std::cout << "资金账户信息:" << std::endl;
            std::cout << "  账户ID:        " << pTradingAccount->AccountID << std::endl;
            std::cout << "  可用资金:       " << pTradingAccount->Available << std::endl;
            std::cout << "  保证金占用:     " << pTradingAccount->CurrMargin << std::endl;
            // std::cout << "  浮动盈亏:       " << pTradingAccount->UnrealizedProfit << std::endl;
            std::cout << "  持仓盈亏:       " << pTradingAccount->CloseProfit << std::endl;
            std::cout << "  权益:           " << pTradingAccount->Balance << std::endl;
            std::cout << "  入金:           " << pTradingAccount->Deposit << std::endl;
            std::cout << "  出金:           " << pTradingAccount->Withdraw << std::endl;
            std::cout << "  冻结保证金:     " << pTradingAccount->FrozenMargin << std::endl;
            std::cout << "  冻结手续费:     " << pTradingAccount->FrozenCommission << std::endl;
            std::cout << "  手续费:         " << pTradingAccount->Commission << std::endl;
            // std::cout << "  风险度:         " << pTradingAccount->RiskRatio << std::endl;
            std::cout << "====================================" << std::endl;
        }

        if (bIsLast) {
            // 查询完成后查询持仓
            std::cout << "[状态] 查询投资者持仓..." << std::endl;
            ReqQryInvestorPosition();
        }
    }

    /// 查询投资者持仓响应
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                          CThostFtdcRspInfoField *pRspInfo,
                                          int nRequestID, bool bIsLast) override {
        static bool firstQuery = true;

        if (pRspInfo && pRspInfo->ErrorID != 0 && pRspInfo->ErrorID != 203) { // 203表示没有持仓
            std::cout << "[错误] 查询持仓失败, ErrorID: " << pRspInfo->ErrorID
                      << ", ErrorMsg: " << pRspInfo->ErrorMsg << std::endl;
        } else if (pInvestorPosition) {
            if (firstQuery) {
                std::cout << "[成功] 查询持仓成功" << std::endl;
                std::cout << "====================================" << std::endl;
                std::cout << "持仓信息:" << std::endl;
                firstQuery = false;
            }
            std::cout << "  合约: " << pInvestorPosition->InstrumentID
                      << " | 方向: " << (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long ? "多头" : "空头")
                      << " | 持仓: " << pInvestorPosition->Position << std::endl;
                      // << " | 可用: " << pInvestorPosition->Available << std::endl;
        }

        if (bIsLast) {
            std::cout << "====================================" << std::endl;
            std::cout << "[状态] 所有查询完成, 登录测试成功!" << std::endl;
            std::cout << "[状态] 按Ctrl+C退出或等待自动登出..." << std::endl;
        }
    }

    /// 错误应答
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo,
                           int nRequestID, bool bIsLast) override {
        std::cout << "[错误] 收到错误响应, RequestID: " << nRequestID << std::endl;
        if (pRspInfo) {
            std::cout << "  ErrorID: " << pRspInfo->ErrorID << std::endl;
            std::cout << "  ErrorMsg: " << (pRspInfo->ErrorMsg[0] ? pRspInfo->ErrorMsg : "无") << std::endl;
        }
    }

    /// 设置登录参数
    void SetLoginInfo(const std::string& frontAddr,
                      const std::string& brokerId,
                      const std::string& userId,
                      const std::string& password,
                      const std::string& appId = "",
                      const std::string& authCode = "") {
        m_frontAddr = frontAddr;
        m_brokerId = brokerId;
        m_userId = userId;
        m_password = password;
        m_appId = appId;
        m_authCode = authCode;
    }

    /// 设置投资者ID
    void SetInvestorId(const std::string& investorId) {
        m_investorId = investorId;
    }

    /// 请求用户登录
    void ReqUserLogin() {
        CThostFtdcReqUserLoginField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.UserID, m_userId.c_str(), sizeof(req.UserID) - 1);
        strncpy(req.Password, m_password.c_str(), sizeof(req.Password) - 1);

        // 如果有认证信息，先进行认证
        if (!m_appId.empty() && !m_authCode.empty()) {
            ReqAuthenticate();
        } else {
            int result = m_api->ReqUserLogin(&req, ++m_requestId);
            if (result == 0) {
                std::cout << "[请求] 发送登录请求, RequestID: " << m_requestId << std::endl;
            } else {
                std::cout << "[错误] 发送登录请求失败, 返回码: " << result << std::endl;
            }
        }
    }

    /// 客户端认证请求
    void ReqAuthenticate() {
        CThostFtdcReqAuthenticateField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.UserID, m_userId.c_str(), sizeof(req.UserID) - 1);
        strncpy(req.AuthCode, m_authCode.c_str(), sizeof(req.AuthCode) - 1);
        strncpy(req.AppID, m_appId.c_str(), sizeof(req.AppID) - 1);

        int result = m_api->ReqAuthenticate(&req, ++m_requestId);
        if (result == 0) {
            std::cout << "[请求] 发送认证请求, RequestID: " << m_requestId << std::endl;
        } else {
            std::cout << "[错误] 发送认证请求失败, 返回码: " << result << std::endl;
        }
    }

    /// 查询结算信息
    void ReqQrySettlementInfo() {
        CThostFtdcQrySettlementInfoField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.InvestorID, m_investorId.c_str(), sizeof(req.InvestorID) - 1);

        int result = m_api->ReqQrySettlementInfo(&req, ++m_requestId);
        if (result == 0) {
            std::cout << "[请求] 发送查询结算信息请求, RequestID: " << m_requestId << std::endl;
        } else {
            std::cout << "[错误] 发送查询结算信息请求失败, 返回码: " << result << std::endl;
        }
    }

    /// 投资者结算结果确认
    void ReqSettlementInfoConfirm() {
        CThostFtdcSettlementInfoConfirmField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.InvestorID, m_investorId.c_str(), sizeof(req.InvestorID) - 1);

        int result = m_api->ReqSettlementInfoConfirm(&req, ++m_requestId);
        if (result == 0) {
            std::cout << "[请求] 发送结算确认请求, RequestID: " << m_requestId << std::endl;
        } else {
            std::cout << "[错误] 发送结算确认请求失败, 返回码: " << result << std::endl;
        }
    }

    /// 查询资金账户
    void ReqQryTradingAccount() {
        CThostFtdcQryTradingAccountField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.InvestorID, m_investorId.c_str(), sizeof(req.InvestorID) - 1);

        int result = m_api->ReqQryTradingAccount(&req, ++m_requestId);
        if (result == 0) {
            std::cout << "[请求] 发送查询资金账户请求, RequestID: " << m_requestId << std::endl;
        } else {
            std::cout << "[错误] 发送查询资金账户请求失败, 返回码: " << result << std::endl;
        }
    }

    /// 查询投资者持仓
    void ReqQryInvestorPosition() {
        CThostFtdcQryInvestorPositionField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.InvestorID, m_investorId.c_str(), sizeof(req.InvestorID) - 1);

        int result = m_api->ReqQryInvestorPosition(&req, ++m_requestId);
        if (result == 0) {
            std::cout << "[请求] 发送查询持仓请求, RequestID: " << m_requestId << std::endl;
        } else {
            std::cout << "[错误] 发送查询持仓请求失败, 返回码: " << result << std::endl;
        }
    }

    /// 请求登出
    void ReqUserLogout() {
        CThostFtdcUserLogoutField req = {0};

        strncpy(req.BrokerID, m_brokerId.c_str(), sizeof(req.BrokerID) - 1);
        strncpy(req.UserID, m_userId.c_str(), sizeof(req.UserID) - 1);

        int result = m_api->ReqUserLogout(&req, ++m_requestId);
        if (result == 0) {
            std::cout << "[请求] 发送登出请求, RequestID: " << m_requestId << std::endl;
        } else {
            std::cout << "[错误] 发送登出请求失败, 返回码: " << result << std::endl;
            g_running = false;
        }
    }

private:
    CThostFtdcTraderApi* m_api;
    int m_requestId;
    std::string m_frontAddr;
    std::string m_brokerId;
    std::string m_userId;
    std::string m_password;
    std::string m_investorId;
    std::string m_appId;
    std::string m_authCode;
};

///
/// @brief 从JSON字符串中提取字段值
///
std::string GetJsonField(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";

    pos = json.find("\"", pos);
    if (pos == std::string::npos) return "";
    pos++; // 跳过引号

    size_t endPos = json.find("\"", pos);
    if (endPos == std::string::npos) return "";

    return json.substr(pos, endPos - pos);
}

/// @brief 从config.json读取配置
///
bool LoadConfigFromFile(std::string& frontAddr, std::string& brokerId,
                        std::string& userId, std::string& password,
                        std::string& investorId, std::string& appId,
                        std::string& authCode) {
    std::ifstream file("config.json");
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();

    std::string tdHost = GetJsonField(json, "tdHost");
    if (!tdHost.empty()) frontAddr = tdHost;

    std::string broker = GetJsonField(json, "brokerId");
    if (!broker.empty()) brokerId = broker;

    std::string investor = GetJsonField(json, "investorId");
    if (!investor.empty()) {
        userId = investor;
        investorId = investor;
    }

    std::string pass = GetJsonField(json, "password");
    if (!pass.empty()) password = pass;

    std::string app = GetJsonField(json, "appId");
    if (!app.empty()) appId = app;

    std::string auth = GetJsonField(json, "authCode");
    if (!auth.empty()) authCode = auth;

    return true;
}

/// @brief 打印使用说明
///
void PrintUsage(const char* programName) {
    std::cout << "使用方法: " << programName << " [选项]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -f <地址>  交易服务器地址 (格式: tcp://ip:port)" << std::endl;
    std::cout << "  -b <经纪商> 经纪公司代码" << std::endl;
    std::cout << "  -u <用户名> 用户名" << std::endl;
    std::cout << "  -p <密码>   密码" << std::endl;
    std::cout << "  -i <投资者> 投资者代码 (默认与用户名相同)" << std::endl;
    std::cout << "  -a <AppID>  应用ID (用于认证)" << std::endl;
    std::cout << "  -c <AuthCode> 认证码" << std::endl;
    std::cout << "  -h          显示帮助信息" << std::endl;
    std::cout << "\n示例:" << std::endl;
    std::cout << "  " << programName << " -f tcp://180.168.146.187:10130 -b 9999 -u test1 -p 123456" << std::endl;
    std::cout << "\n或使用默认配置 (simnow测试环境):" << std::endl;
    std::cout << "  " << programName << std::endl;
}

///
/// @brief 主函数
///
int main(int argc, char* argv[]) {
    std::cout << "====================================" << std::endl;
    std::cout << "  CTP交易API登录测试程序" << std::endl;
    std::cout << "  版本: 1.0.0" << std::endl;
    std::cout << "====================================" << std::endl;

    // 注册信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // 默认配置 (SimNow 7x24测试环境)
    std::string frontAddr = "tcp://180.168.146.187:10130";  // 电信
    std::string brokerId = "9999";
    std::string userId = "";
    std::string password = "";
    std::string investorId = "";
    std::string appId = "";
    std::string authCode = "";

    // 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "f:b:u:p:i:a:c:h")) != -1) {
        switch (opt) {
            case 'f':
                frontAddr = optarg;
                break;
            case 'b':
                brokerId = optarg;
                break;
            case 'u':
                userId = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'i':
                investorId = optarg;
                break;
            case 'a':
                appId = optarg;
                break;
            case 'c':
                authCode = optarg;
                break;
            case 'h':
                PrintUsage(argv[0]);
                return 0;
            default:
                PrintUsage(argv[0]);
                return 1;
        }
    }

    // 如果没有命令行参数，尝试从config.json读取配置
    if (userId.empty() && password.empty()) {
        if (LoadConfigFromFile(frontAddr, brokerId, userId, password, investorId, appId, authCode)) {
            std::cout << "[状态] 已从 config.json 加载配置" << std::endl;
        }
    }

    // 检查必需参数
    if (userId.empty()) {
        std::cout << "[错误] 请指定用户名 (-u 参数)" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }

    if (password.empty()) {
        std::cout << "[错误] 请指定密码 (-p 参数)" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }

    // 投资者ID默认与用户名相同
    if (investorId.empty()) {
        investorId = userId;
    }

    std::cout << "[配置] 连接配置:" << std::endl;
    std::cout << "  前端地址: " << frontAddr << std::endl;
    std::cout << "  经纪公司: " << brokerId << std::endl;
    std::cout << "  用户名:   " << userId << std::endl;
    std::cout << "  投资者:   " << investorId << std::endl;
    std::cout << "====================================" << std::endl;

    // 创建交易API实例
    std::cout << "[状态] 创建交易API实例..." << std::endl;
    CThostFtdcTraderApi* traderApi = CThostFtdcTraderApi::CreateFtdcTraderApi("./flow/");

    // 创建并注册回调实例
    TraderSpi traderSpi(traderApi);
    traderSpi.SetLoginInfo(frontAddr, brokerId, userId, password, appId, authCode);
    traderSpi.SetInvestorId(investorId);
    traderApi->RegisterSpi(&traderSpi);

    // 订阅私有流和公共流
    traderApi->SubscribePrivateTopic(THOST_TERT_RESTART);
    traderApi->SubscribePublicTopic(THOST_TERT_RESTART);

    // 注册前端地址
    std::cout << "[状态] 注册交易服务器地址..." << std::endl;
    traderApi->RegisterFront(const_cast<char*>(frontAddr.c_str()));

    // 初始化
    std::cout << "[状态] 初始化交易API..." << std::endl;
    traderApi->Init();

    std::cout << "[状态] 等待连接..." << std::endl;

    // 主循环
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 登出
    std::cout << "[状态] 正在登出..." << std::endl;
    traderSpi.ReqUserLogout();

    // 等待登出完成
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 释放资源
    std::cout << "[状态] 释放资源..." << std::endl;
    traderApi->Release();

    std::cout << "[完成] 程序退出" << std::endl;
    return 0;
}

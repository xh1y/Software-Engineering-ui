#pragma once

#include <QString>
#include <QColor>
#include <QMap>
#include <QStringList>
#include <QSettings>

namespace optikg {

/**
 * @brief 应用程序常量类
 *
 * 包含所有硬编码的常量值，提供统一的默认值管理。
 * 这些常量可以作为配置的默认值，部分可以通过 ConfigManager 进行配置。
 */
class AppConstants {
public:
    // 禁用实例化
    AppConstants() = delete;
    
    /**
     * @brief 初始化可配置参数（从配置文件加载）
     */
    static void initialize();
    
    /**
     * @brief 保存当前配置到配置文件
     */
    static void saveSettings();
    
    /**
     * @brief 重置所有可配置参数为默认值
     */
    static void resetToDefaults();
    
    // 可配置参数的 getter/setter
    static int chunkSize();
    static void setChunkSize(int size);
    static int overlapSize();
    static void setOverlapSize(int size);
    static int maxChars();
    static void setMaxChars(int max);

    // ============ UI 常量 ============

    struct UI {
        // 窗口尺寸
        static constexpr int WINDOW_WIDTH = 1200;
        static constexpr int WINDOW_HEIGHT = 800;

        // 批量处理对话框尺寸
        static constexpr int BATCH_DIALOG_WIDTH = 800;
        static constexpr int BATCH_DIALOG_HEIGHT = 700;

        // 超时时间（毫秒）
        static constexpr int TOAST_TIMEOUT_MS = 3000;

        // 状态消息颜色
        static const QString ERROR_COLOR;     // #f44336
        static const QString SUCCESS_COLOR;   // #4CAF50
        static const QString WARNING_COLOR;   // #FF9800
        static const QString INFO_COLOR;      // #666

        // 按钮颜色
        static const QString EXTRACT_BTN_COLOR;      // #4CAF50
        static const QString EXTRACT_BTN_HOVER;      // #388E3C
        static const QString EXTRACT_BTN_DISABLED;   // #BDBDBD
        static const QString CLEAR_BTN_COLOR;        // #f44336
        static const QString CLEAR_BTN_HOVER;        // #d32f2f
        static const QString BATCH_DELETE_BTN_COLOR; // #f44336

        // 文本颜色
        static const QString CHAR_COUNT_COLOR;       // #666
        static const QString CHAR_COUNT_WARNING;     // #FF9800
        static const QString CHAR_COUNT_ERROR;       // #f44336

        // 字体大小
        static constexpr int CHAR_COUNT_FONT_SIZE = 12;

        // 分割器比例
        static constexpr int LEFT_SPLITTER_FACTOR1 = 3;  // 抽取面板
        static constexpr int LEFT_SPLITTER_FACTOR2 = 1;  // 历史记录
        static constexpr int RIGHT_SPLITTER_FACTOR1 = 2; // 知识图谱
        static constexpr int RIGHT_SPLITTER_FACTOR2 = 1; // 结果列表

        // 进度条范围
        static constexpr int PROGRESS_MIN = 0;
        static constexpr int PROGRESS_MAX = 100;

        // 结果树列宽
        static constexpr int RESULT_TREE_TYPE_COLUMN_WIDTH = 100;
    };

    // ============ 模型处理常量 ============

    struct Model {
        // 文本分块参数（现在是可配置的，使用getter访问当前值）
        static constexpr int CHUNK_THRESHOLD = 450;           // 字符数阈值（固定）
        static constexpr int DEFAULT_CHUNK_SIZE_DEFAULT = 500;    // 默认块大小默认值
        static constexpr int DEFAULT_OVERLAP_SIZE_DEFAULT = 100;  // 默认重叠大小默认值
        static constexpr int MAX_CHARS_DEFAULT = 10000;           // 最大字符数默认值
        
        // 运行时配置值（通过initialize()从配置文件加载）
        static int& DEFAULT_CHUNK_SIZE();
        static int& DEFAULT_OVERLAP_SIZE();
        static int& MAX_CHARS();

        // 默认阈值
        static constexpr float DEFAULT_THRESHOLD = -0.2f;

        // 阈值范围
        static constexpr float THRESHOLD_MIN = -100.0f;
        static constexpr float THRESHOLD_MAX = 0.0f;
        static constexpr float THRESHOLD_STEP = 1.0f;

        // 验证参数有效性
        static bool validateChunkSize(int size);
        static bool validateOverlapSize(int size);
        static bool validateMaxChars(int max);
    };

    // ============ 文件处理常量 ============

    struct File {
        // 文件扩展名
        static const QString JSON_EXTENSION;           // .json
        static const QString CSV_EXTENSION;            // .csv

        // 文件过滤器
        static const QString JSON_FILE_FILTER;         // JSON文件 (*.json)
        static const QString CSV_FILE_FILTER;          // CSV文件 (*.csv)
        static const QString ONNX_FILE_FILTER;         // ONNX 模型 (*.onnx)

        // CSV 编码选项
        static QStringList getCsvEncodings();
    };

    // ============ 字段映射默认值 ============

    struct Mapping {
        static constexpr const char* DEFAULT_JSON_FIELD = "text";
        static constexpr const char* DEFAULT_CSV_COLUMN = "content";
        static constexpr const char* DEFAULT_CSV_ENCODING = "UTF-8";
        static constexpr const char* DEFAULT_EXPORT_FORMAT = "JSON";
    };

    // ============ 图形布局常量 ============

    struct Graph {
        // 场景矩形
        static constexpr qreal SCENE_RECT_X = -400.0;
        static constexpr qreal SCENE_RECT_Y = -300.0;
        static constexpr qreal SCENE_RECT_WIDTH = 800.0;
        static constexpr qreal SCENE_RECT_HEIGHT = 600.0;

        // 力导向布局参数
        static constexpr int FORCE_ITERATIONS = 100;
        static constexpr qreal REPULSION_FORCE = 100.0;
        static constexpr qreal ATTRACTION_FORCE = 0.1;
        static constexpr qreal DAMPING = 0.8;

        // 节点参数
        static constexpr qreal NODE_RADIUS = 30.0;
        static constexpr qreal NODE_SPACING = 80.0;
    };

    // ============ 数据库常量 ============

    struct Database {
        // 历史记录查询限制
        static constexpr int HISTORY_LIMIT = 100;
    };

    // ============ 应用程序信息 ============

    struct AppInfo {
        // 配置文件相关
        static const QString CONFIG_ORG;        // OptiKG
        static const QString CONFIG_APP;        // config
    };
};

} // namespace optikg
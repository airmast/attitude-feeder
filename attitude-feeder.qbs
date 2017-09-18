import qbs

Project {
    property string appName: "attfeeder"
    property int versionMajor: 0
    property int versionMinor: 0
    property int versionRelease: 1

    minimumQbsVersion: "1.6.0"

    references: [
        "src/src.qbs"
    ]
}

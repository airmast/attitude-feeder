import qbs

Project {
    property string appName: "attfeeder"
    property int versionMajor: 0
    property int versionMinor: 0
    property int versionRelease: 1

    minimumQbsVersion: "1.4.5"

    references: [
        "src/src.qbs"
    ]
}

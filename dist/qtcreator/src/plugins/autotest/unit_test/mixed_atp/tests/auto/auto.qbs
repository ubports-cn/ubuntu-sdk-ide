import qbs

Project {
    name: "Auto tests"

    references: [
        "bench/bench.qbs",
        "dummy/dummy.qbs",
        "gui/gui.qbs",
        "quickauto/quickauto.qbs",
        "quickauto2/quickauto2.qbs"
    ]
}

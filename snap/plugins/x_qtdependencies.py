import snapcraft


class QtdependenciesPlugin(snapcraft.BasePlugin):
    def __init__(self, name, options, project):
        super().__init__(name, options, project)
        self.build_packages.extend(['libqt5serialport5-dev', 'libqt5network5', 'qtscript5-dev'])

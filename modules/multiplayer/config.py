def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "SceneReplicationConfig",
        "MultiplayerSpawner",
        "MultiplayerSynchronizer",
        "OfflineMultiplayerPeer",
    ]


def get_doc_path():
    return "doc_classes"

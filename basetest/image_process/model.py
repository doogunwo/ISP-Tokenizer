import torch.nn as nn
from torchvision import models

def build_model(num_classes, pretrained=False):
    """
    ResNet-18 기반 모델을 num_classes 출력으로 설정.
    """
    model = models.resnet18(pretrained=pretrained)
    # 마지막 fc 레이어 교체
    in_features = model.fc.in_features
    model.fc = nn.Linear(in_features, num_classes)
    return model

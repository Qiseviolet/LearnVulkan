float perspectiveCameraDepthLinear(float glFragCoordZ, float near, float far){
    //还原线性视空间深度
    float viewZ = (near * far) / (far - glFragCoordZ * (far - near));
    float linear01 = (viewZ - near) / (far - near);
    return clamp(linear01, 0.0, 1.0);
}

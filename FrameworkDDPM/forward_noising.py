import torch
import torch.nn.functional as F


def linear_beta_schedule(timesteps, start=0.0001, end=0.02):
    return torch.linspace(start, end, timesteps)


def get_index_from_list(vals, time_step, x_shape):
    """
    Returns a specific index t of a passed list of values vals
    while considering the batch dimension.
    """
    batch_size = time_step.shape[0]
    values = vals.to(time_step.device)
    out = values.gather(0, time_step)
    reshape_dims = (batch_size,) + (1,) * (len(x_shape) - 1)
    return out.reshape(reshape_dims)


# Define beta schedule
T = 300
betas = linear_beta_schedule(timesteps=T)

# Pre-calculate different hyperparameters (alpha and beta) for closed form
alphas = 1.0 - betas
alphas_cumprod = torch.cumprod(alphas, axis=0)
alphas_cumprod_prev = F.pad(alphas_cumprod[:-1], (1, 0), value=1.0)
sqrt_recip_alphas = torch.sqrt(1.0 / alphas)
sqrt_alphas_cumprod = torch.sqrt(alphas_cumprod)
sqrt_one_minus_alphas_cumprod = torch.sqrt(1.0 - alphas_cumprod)
posterior_variance = (
    betas * (1.0 - alphas_cumprod_prev) / (1.0 - alphas_cumprod)
)


def forward_diffusion_sample(x_0, time_step, device="cpu"):
    target_device = torch.device(device)
    x_0 = x_0.to(target_device)
    time_step = time_step.to(target_device)
    noise = torch.randn_like(x_0)

    # q(x_t | x_0) = sqrt(alpha_bar_t) * x_0 + sqrt(1 - alpha_bar_t) * epsilon
    sqrt_alphas_cumprod_t = get_index_from_list(
        sqrt_alphas_cumprod, time_step, x_0.shape
    )
    sqrt_one_minus_alphas_cumprod_t = get_index_from_list(
        sqrt_one_minus_alphas_cumprod, time_step, x_0.shape
    )
    x_noisy = (
        sqrt_alphas_cumprod_t * x_0
        + sqrt_one_minus_alphas_cumprod_t * noise
    )

    return x_noisy, noise

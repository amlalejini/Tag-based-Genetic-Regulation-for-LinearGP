data_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/data/mc-reg/"
fname <- "response_patterns.csv"

dump_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/analysis/"

response_data_loc <- paste(data_dir, fname, sep="")
response_data <-read.csv(response_data_loc, na.strings="NONE")

# Make sure responses/active fields are treated as factors
response_data$response <- factor(response_data$response)
response_data$response__ko_regulation <- factor(response_data$response__ko_regulation)
response_data$response__ko_global_memory <- factor(response_data$response__ko_global_memory)
response_data$response__ko_messaging <- factor(response_data$response__ko_messaging)
response_data$response__ko_regulation_imprinting <- factor(response_data$response__ko_regulation_imprinting)
response_data$response__ko_reproduction_tag_control <- factor(response_data$response__ko_reproduction_tag_control)

response_data$active <- factor(response_data$active)
response_data$active__ko_regulation <- factor(response_data$active__ko_regulation)
response_data$active__ko_global_memory <- factor(response_data$active__ko_global_memory)
response_data$active__ko_messaging <- factor(response_data$active__ko_messaging)
response_data$active__ko_regulation_imprinting <- factor(response_data$active__ko_regulation_imprinting)
response_data$active__ko_reproduction_tag_control <- factor(response_data$active__ko_reproduction_tag_control)

seeds <- 102001:102200
# seeds <- 102002:102002
# Best? 2014
dump_loc <- paste(dump_dir, "imgs/resp_patterns/", sep="")
for (seed in seeds) {
  deme_height <- filter(response_data, SEED==seed)$DEME_HEIGHT[1]
  deme_width <- filter(response_data, SEED==seed)$DEME_WIDTH[1]
  num_resps <- filter(response_data, SEED==seed)$NUM_RESPONSE_TYPES[1]
  response_types <- -1:(num_resps-1)
  
  seed_data <- filter(response_data, SEED==seed)
  seed_data$response <- factor(seed_data$response, levels=response_types)
  seed_data$response__ko_regulation <- factor(seed_data$response__ko_regulation, levels=response_types)
  seed_data$response__ko_global_memory <- factor(seed_data$response__ko_global_memory, levels=response_types)
  seed_data$response__ko_messaging <- factor(seed_data$response__ko_messaging, levels=response_types)
  seed_data$response__ko_regulation_imprinting <- factor(seed_data$response__ko_regulation_imprinting, levels=response_types)
  seed_data$response__ko_reproduction_tag_control <- factor(seed_data$response__ko_reproduction_tag_control, levels=response_types)
  
  USE_FUNC_REGULATION <- seed_data$USE_FUNC_REGULATION[1]
  USE_GLOBAL_MEMORY <- seed_data$USE_GLOBAL_MEMORY[1]
  score <- seed_data$score[1]
  relies_on_reg <- seed_data$relies_on_reg[1]
  relies_on_mem <- seed_data$relies_on_mem[1]
  relies_on_msg <- seed_data$relies_on_msg[1]
  relies_on_imprinting <- seed_data$relies_on_imprinting[1]
  relies_on_repro_tag <- seed_data$relies_on_repro_tag[1]

  out_path <- paste(dump_loc, "resp-pattern-seed-", seed, "-baseline.png", sep="")
  title_info <- paste("Regulation: ", USE_FUNC_REGULATION, 
                      "; Memory: ", USE_GLOBAL_MEMORY, 
                      "; Baseline Score: ", score, 
                      ";\n reg? ", relies_on_reg,
                      "; mem? ", relies_on_mem,
                      "; msg? ", relies_on_msg,
                      "; imprinting? ", relies_on_imprinting,
                      "; repro_tag? ", relies_on_repro_tag,
                      sep="")
  ggplot(seed_data, aes(x=x, y=y)) +
    geom_tile(color="black", aes(fill=response)) +
    scale_fill_discrete(name="Response", limits=response_types, labels=response_types) +
    theme(legend.position = "bottom") +
    scale_y_continuous(breaks=seq(0, deme_height-1, 1)) +
    scale_x_continuous(breaks=seq(0, deme_width-1, 1)) +
    ggtitle(title_info) +
    ggsave(out_path, width=6, height=6)
  
  out_path <- paste(dump_loc, "resp-pattern-seed-", seed, "-ko-reg.png", sep="")
  ggplot(seed_data, aes(x=x, y=y)) +
    geom_tile(color="black", aes(fill=response__ko_regulation)) +
    scale_fill_discrete(name="Response", limits=response_types, labels=response_types) +
    theme(legend.position = "bottom") +
    scale_y_continuous(breaks=seq(0, deme_height-1, 1)) +
    scale_x_continuous(breaks=seq(0, deme_width-1, 1)) +
    ggtitle(title_info) +
    ggsave(out_path, width=6, height=6)
  
  out_path <- paste(dump_loc, "resp-pattern-seed-", seed, "-ko-msg.png", sep="")
  ggplot(seed_data, aes(x=x, y=y)) +
    geom_tile(color="black", aes(fill=response__ko_messaging)) +
    scale_fill_discrete(name="Response", limits=response_types, labels=response_types) +
    theme(legend.position = "bottom") +
    scale_y_continuous(breaks=seq(0, deme_height-1, 1)) +
    scale_x_continuous(breaks=seq(0, deme_width-1, 1)) +
    ggtitle(title_info) +
    ggsave(out_path, width=6, height=6)
  
  out_path <- paste(dump_loc, "resp-pattern-seed-", seed, "-ko-repro-tag.png", sep="")
  ggplot(seed_data,aes(x=x, y=y)) +
    geom_tile(color="black", aes(fill=response__ko_reproduction_tag_control)) +
    scale_fill_discrete(name="Response", limits=response_types, labels=response_types) +
    theme(legend.position = "bottom") +
    scale_y_continuous(breaks=seq(0, deme_height-1, 1)) +
    scale_x_continuous(breaks=seq(0, deme_width-1, 1)) +
    ggtitle(title_info) +
    ggsave(out_path, width=6, height=6)
  
  out_path <- paste(dump_loc, "resp-pattern-seed-", seed, "-ko-imprinting.png", sep="")
  ggplot(seed_data, aes(x=x, y=y)) +
    geom_tile(color="black", aes(fill=response__ko_regulation_imprinting)) +
    scale_fill_discrete(name="Response", limits=response_types, labels=response_types) +
    theme(legend.position = "bottom") +
    scale_y_continuous(breaks=seq(0, deme_height-1, 1)) +
    scale_x_continuous(breaks=seq(0, deme_width-1, 1)) +
    ggtitle(title_info) +
    ggsave(out_path, width=6, height=6)
}
